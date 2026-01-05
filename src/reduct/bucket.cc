// Copyright 2022-2024 ReductSoftware UG

#include "reduct/bucket.h"
#define FMT_HEADER_ONLY 1
#include <fmt/core.h>
#ifdef CONCURRENTQUEUE_H_FILEPATH
#include CONCURRENTQUEUE_H_FILEPATH
#else
#include <moodycamel/concurrentqueue.h>
#endif

#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <deque>
#include <future>
#include <optional>
#include <mutex>
#include <thread>

#include "reduct/internal/batch_v1.h"
#include "reduct/internal/batch_v2.h"
#include "reduct/internal/http_client.h"
#include "reduct/internal/serialisation.h"

namespace reduct {

using internal::IHttpClient;
using internal::ParseStatus;
using internal::QueryOptionsToJsonString;

class Bucket : public IBucket {
  using BatchType = internal::BatchType;

 public:
  Bucket(std::string_view url, std::string_view name, const HttpOptions& options)
      : path_(fmt::format("/b/{}", name)), io_path_(fmt::format("/io/{}", name)), stop_{} {
    name_ = name;
    client_ = IHttpClient::Build(url, options);

    worker_ = std::thread([this] {
      while (!stop_) {
        Task task;
        if (task_queue_.try_dequeue(task)) {
          task();
        } else {
          std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
      }
    });
  }

  ~Bucket() override {
    stop_ = true;
    if (worker_.joinable()) {
      worker_.join();
    }
  }

  Result<Settings> GetSettings() const noexcept override {
    auto [body, err] = client_->Get(path_);
    if (err) {
      return {{}, std::move(err)};
    }

    try {
      auto data = nlohmann::json::parse(body);
      return internal::ParseBucketSettings(data.at("settings"));
    } catch (const std::exception& ex) {
      return {{}, Error{.code = -1, .message = ex.what()}};
    }
  }

  Error UpdateSettings(const Settings& settings) const noexcept override {
    return client_->Put(path_, internal::BucketSettingToJsonString(settings).dump());
  }

  Result<BucketInfo> GetInfo() const noexcept override {
    auto [body, err] = client_->Get(path_);
    if (err) {
      return {{}, std::move(err)};
    }

    try {
      auto info = nlohmann::json::parse(body).at("info");
      return {
          BucketInfo{
              .name = info.at("name"),
              .entry_count = info.at("entry_count"),
              .size = info.at("size"),
              .oldest_record = Time() + std::chrono::microseconds(info.at("oldest_record")),
              .latest_record = Time() + std::chrono::microseconds(info.at("latest_record")),
              .is_provisioned = info.value("is_provisioned", false),
              .status = ParseStatus(info),
          },
          Error::kOk,
      };
    } catch (const std::exception& e) {
      return {{}, Error{.code = -1, .message = e.what()}};
    }
  }

  Result<std::vector<EntryInfo>> GetEntryList() const noexcept override {
    auto [body, err] = client_->Get(path_);
    if (err) {
      return {{}, std::move(err)};
    }

    try {
      auto json_entries = nlohmann::json::parse(body).at("entries");
      std::vector<EntryInfo> entries(json_entries.size());
      for (int i = 0; i < entries.size(); ++i) {
        auto entry = json_entries[i];
        entries[i] = EntryInfo{
            .name = entry.at("name"),
            .record_count = entry.at("record_count"),
            .block_count = entry.at("block_count"),
            .size = entry.at("size"),
            .oldest_record = Time() + std::chrono::microseconds(entry.at("oldest_record")),
            .latest_record = Time() + std::chrono::microseconds(entry.at("latest_record")),
            .status = ParseStatus(entry),
        };
      }

      return {entries, Error::kOk};
    } catch (const std::exception& e) {
      return {{}, Error{.code = -1, .message = e.what()}};
    }
  }

  Error Remove() const noexcept override { return client_->Delete(path_); }

  Error RemoveEntry(std::string_view entry_name) const noexcept override {
    return client_->Delete(fmt::format("{}/{}", path_, entry_name));
  }

  Error RemoveRecord(std::string_view entry_name, Time timestamp) const noexcept override {
    return client_->Delete(fmt::format("{}/{}?ts={}", path_, entry_name, internal::ToMicroseconds(timestamp)));
  }

  Error Write(std::string_view entry_name, std::optional<Time> ts,
              WriteRecordCallback callback) const noexcept override {
    return Write(entry_name, {.timestamp = ts}, std::move(callback));
  }

  Error Write(std::string_view entry_name, const WriteOptions& options,
              WriteRecordCallback callback) const noexcept override {
    WritableRecord record;
    callback(&record);

    const auto time =
        options.timestamp ? internal::ToMicroseconds(*options.timestamp) : internal::ToMicroseconds(Time::clock::now());
    const auto content_type = options.content_type.empty() ? "application/octet-stream" : options.content_type;

    IHttpClient::Headers headers = MakeHeadersFromLabels(options);
    return client_->Post(fmt::format("{}/{}?ts={}", path_, entry_name, time), content_type, record.content_length_,
                         std::move(headers), std::move(record.callback_));
  }

  Result<BatchErrors> WriteBatch(std::string_view entry_name, BatchCallback callback) const noexcept override {
    return ProcessBatch(entry_name, std::move(callback), BatchType::kWrite);
  }

  Result<BatchErrors> UpdateBatch(std::string_view entry_name, BatchCallback callback) const noexcept override {
    return ProcessBatch(entry_name, std::move(callback), BatchType::kUpdate);
  }

  Result<BatchErrors> RemoveBatch(std::string_view entry_name, BatchCallback callback) const noexcept override {
    return ProcessBatch(entry_name, std::move(callback), BatchType::kRemove);
  }

  Error Update(std::string_view entry_name, const WriteOptions& options) const noexcept override {
    if (!options.timestamp) {
      return Error{.code = 400, .message = "Timestamp is required"};
    }

    const auto time = internal::ToMicroseconds(*options.timestamp);
    IHttpClient::Headers headers = MakeHeadersFromLabels(options);
    return client_->Patch(fmt::format("{}/{}?ts={}", path_, entry_name, time), "", std::move(headers));
  }

  Error Read(std::string_view entry_name, std::optional<Time> ts, ReadRecordCallback callback) const noexcept override {
    auto path = fmt::format("{}/{}", path_, entry_name);
    if (ts) {
      path.append(fmt::format("?ts={}", internal::ToMicroseconds(*ts)));
    }

    auto record_err = ReadRecord(std::move(path), ReadType::kSingle, false, callback);
    return record_err.error;
  }

  Error Head(std::string_view entry_name, std::optional<Time> ts, ReadRecordCallback callback) const noexcept override {
    auto path = fmt::format("{}/{}", path_, entry_name);
    if (ts) {
      path.append(fmt::format("?ts={}", internal::ToMicroseconds(*ts)));
    }

    auto record_err = ReadRecord(std::move(path), ReadType::kSingle, true, callback);
    return record_err.error;
  }

  Error Query(std::string_view entry_name, std::optional<Time> start, std::optional<Time> stop, QueryOptions options,
              ReadRecordCallback callback) const noexcept override {
    if (SupportsBatchProtocolV2()) {
      return QueryV2(entry_name, start, stop, options, callback);
    }

    return QueryV1(entry_name, start, stop, options, callback);
  }

  Error QueryV1(std::string_view entry_name, std::optional<Time> start, std::optional<Time> stop, QueryOptions options,
                const ReadRecordCallback& callback) const {
    std::string body;
    auto [json_payload, json_err] = QueryOptionsToJsonString("QUERY", start, stop, options);
    if (json_err) {
      return json_err;
    }

    auto [resp, resp_err] = client_->PostWithResponse(fmt::format("{}/{}/q", path_, entry_name), json_payload.dump());
    if (resp_err) {
      return resp_err;
    }

    body = std::move(resp);

    uint64_t id;
    try {
      auto data = nlohmann::json::parse(body);
      id = data["id"];
    } catch (const std::exception& ex) {
      return Error{.code = -1, .message = ex.what()};
    }

    while (true) {
      auto [stopped, record_err] = ReadRecord(fmt::format("{}/{}/batch?q={}", path_, entry_name, id),
                                              ReadType::kBatched, options.head_only, callback);

      if (stopped) {
        break;
      }

      if (record_err) {
        if (record_err.code == 204) {
          if (options.continuous) {
            std::this_thread::sleep_for(options.poll_interval);
            continue;
          }
          break;
        }

        return record_err;
      }
    }

    return Error::kOk;
  }

  Error QueryV2(std::string_view entry_name, std::optional<Time> start, std::optional<Time> stop, QueryOptions options,
                const ReadRecordCallback& callback) const {
    auto [json_payload, json_err] = QueryOptionsToJsonString("QUERY", start, stop, options);
    if (json_err) {
      return json_err;
    }
    json_payload["entries"] = nlohmann::ordered_json::array({entry_name});

    auto [resp, resp_err] = client_->PostWithResponse(fmt::format("{}/q", io_path_), json_payload.dump());
    if (resp_err) {
      return resp_err;
    }

    uint64_t id;
    try {
      auto data = nlohmann::json::parse(resp);
      id = data["id"];
    } catch (const std::exception& ex) {
      return Error{.code = -1, .message = ex.what()};
    }

    while (true) {
      auto [stopped, record_err] = ReadRecordV2(id, options.head_only, callback);

      if (stopped) {
        break;
      }

      if (record_err) {
        if (record_err.code == 204) {
          if (options.continuous) {
            std::this_thread::sleep_for(options.poll_interval);
            continue;
          }
          break;
        }

        return record_err;
      }
    }

    return Error::kOk;
  }

  Result<uint64_t> RemoveQuery(std::string_view entry_name, std::optional<Time> start, std::optional<Time> stop,
                               QueryOptions options) const noexcept override {
    if (SupportsBatchProtocolV2()) {
      return RemoveQueryV2(entry_name, start, stop, options);
    }

    return RemoveQueryV1(entry_name, start, stop, options);
  }

  Result<uint64_t> RemoveQueryV1(std::string_view entry_name, std::optional<Time> start, std::optional<Time> stop,
                                 QueryOptions options) const {
    std::string body;
    auto [json_payload, json_err] = QueryOptionsToJsonString("REMOVE", start, stop, options);
    if (json_err) {
      return {0, std::move(json_err)};
    }

    auto [resp, resp_err] = client_->PostWithResponse(fmt::format("{}/{}/q", path_, entry_name), json_payload.dump());
    if (resp_err) {
      return {0, std::move(resp_err)};
    }

    body = std::move(resp);

    try {
      auto data = nlohmann::json::parse(body);
      return {data.at("removed_records"), Error::kOk};
    } catch (const std::exception& ex) {
      return {0, Error{.code = -1, .message = ex.what()}};
    }
  }

  Result<uint64_t> RemoveQueryV2(std::string_view entry_name, std::optional<Time> start, std::optional<Time> stop,
                                 QueryOptions options) const {
    auto [json_payload, json_err] = QueryOptionsToJsonString("REMOVE", start, stop, options);
    if (json_err) {
      return {0, std::move(json_err)};
    }
    json_payload["entries"] = nlohmann::ordered_json::array({entry_name});

    auto [resp, resp_err] = client_->PostWithResponse(fmt::format("{}/q", io_path_), json_payload.dump());
    if (resp_err) {
      return {0, std::move(resp_err)};
    }

    try {
      auto data = nlohmann::json::parse(resp);
      return {data.at("removed_records"), Error::kOk};
    } catch (const std::exception& ex) {
      return {0, Error{.code = -1, .message = ex.what()}};
    }
  }

  Error RenameEntry(std::string_view old_name, std::string_view new_name) const noexcept override {
    nlohmann::json data;
    data["new_name"] = new_name;
    return client_->Put(fmt::format("{}/{}/rename", path_, old_name), data.dump());
  }

  Error Rename(std::string_view new_name) noexcept override {
    nlohmann::json data;
    data["new_name"] = new_name;
    auto err = client_->Put(fmt::format("{}/rename", path_), data.dump());
    if (err) {
      return err;
    }
    path_ = fmt::format("/b/{}", new_name);
    io_path_ = fmt::format("/io/{}", new_name);
    return Error::kOk;
  }

  Result<std::string> CreateQueryLink(std::string entry_name, QueryLinkOptions options) const noexcept override {
    auto [json_payload, json_err] = internal::QueryLinkOptionsToJsonString(name_, entry_name, options);

    auto file_name =
        options.file_name ? *options.file_name : fmt::format("{}_{}.bin", entry_name, options.record_index);
    auto [body, err] = client_->PostWithResponse(fmt::format("/links/{}", file_name),
                                                 json_payload.dump());
    if (err) {
      return {{}, std::move(err)};
    }

    try {
      auto data = nlohmann::json::parse(body);
      return {data.at("link").get<std::string>(), Error::kOk};
    } catch (const std::exception& ex) {
      return {{}, Error{.code = -1, .message = ex.what()}};
    }
  }

 private:
  enum class ReadType {
    kSingle,
    kBatched,
  };

  Result<bool> ReadRecord(std::string&& path, ReadType type, bool head,
                          const ReadRecordCallback& callback) const noexcept {
    std::deque<std::optional<std::string>> data;
    std::mutex data_mutex;
    std::future<void> future;
    bool stopped = false;

    auto parse_headers_and_receive_data = [&type, &stopped, &data, &data_mutex, &callback, &future, head,
                                           this](IHttpClient::Headers&& headers) {
      std::vector<ReadableRecord> records;
      if (type == ReadType::kBatched) {
        records = internal::ParseAndBuildBatchedRecordsV1(&data, &data_mutex, head, std::move(headers));
      } else {
        records.emplace_back(ParseAndBuildSingleRecord(&data, &data_mutex, head, std::move(headers)));
      }
      for (auto& record : records) {
        Task task([record = std::move(record), &callback, &stopped] {
          if (stopped) {
            return;
          }
          stopped = !callback(record);
          if (!stopped) {
            stopped = record.last;
          }
        });

        future = task.get_future();
        task_queue_.enqueue(std::move(task));
      }
    };

    Error err;
    if (head) {
      auto ret = client_->Head(path);
      if (ret.error) {
        err = ret.error;
      } else {
        parse_headers_and_receive_data(std::move(ret.result));
      }
    } else {
      err = client_->Get(path, parse_headers_and_receive_data, [&data, &data_mutex](auto chunk) {
        {
          std::lock_guard lock(data_mutex);
          data.emplace_back(std::string(chunk));
        }
        return true;
      });
    }

    if (!err) {
      if (!head) {
        // we use nullptr to indicate the end of the stream
        std::lock_guard lock(data_mutex);
        data.emplace_back(std::nullopt);
      }

      if (future.valid()) {
        future.wait();
      }
    }

    return {stopped, err};
  }

  Result<bool> ReadRecordV2(uint64_t query_id, bool head, const ReadRecordCallback& callback) const noexcept {
    std::deque<std::optional<std::string>> data;
    std::mutex data_mutex;
    std::future<void> future;
    bool stopped = false;

    IHttpClient::Headers request_headers;
    request_headers.emplace("x-reduct-query-id", std::to_string(query_id));

    auto parse_headers_and_receive_data = [&stopped, &data, &data_mutex, &callback, &future, head,
                                           this](IHttpClient::Headers&& headers) {
      auto records = internal::ParseAndBuildBatchedRecordsV2(&data, &data_mutex, head, std::move(headers));
      for (auto& record : records) {
        Task task([record = std::move(record), &callback, &stopped] {
          if (stopped) {
            return;
          }
          stopped = !callback(record);
          if (!stopped) {
            stopped = record.last;
          }
        });

        future = task.get_future();
        task_queue_.enqueue(std::move(task));
      }
    };

    Error err;
    if (head) {
      auto ret = client_->Head(fmt::format("{}/read", io_path_), std::move(request_headers));
      if (ret.error) {
        err = ret.error;
      } else {
        parse_headers_and_receive_data(std::move(ret.result));
      }
    } else {
      err = client_->Get(fmt::format("{}/read", io_path_), std::move(request_headers),
                         parse_headers_and_receive_data, [&data, &data_mutex](auto chunk) {
                           {
                             std::lock_guard lock(data_mutex);
                             data.emplace_back(std::string(chunk));
                           }
                           return true;
                         });
    }

    if (!err) {
      if (!head) {
        std::lock_guard lock(data_mutex);
        data.emplace_back(std::nullopt);
      }

      if (future.valid()) {
        future.wait();
      }
    }

    return {stopped, err};
  }

  static ReadableRecord ParseAndBuildSingleRecord(std::deque<std::optional<std::string>>* data, std::mutex* mutex,
                                                  bool head, IHttpClient::Headers&& headers) {
    ReadableRecord record;

    record.timestamp = internal::FromMicroseconds(headers["x-reduct-time"]);
    record.size = std::stoi(headers["content-length"]);
    record.content_type = headers["content-type"];
    record.last = headers["x-reduct-last"] == "1";

    for (const auto& [key, value] : headers) {
      if (key.starts_with("x-reduct-label-")) {
        record.labels.emplace(key.substr(15), value);
      }
    }

    record.Read = [data, mutex, head](auto record_callback) {
      if (head) {
        return Error::kOk;
      }

      while (true) {
        std::optional<std::string> chunk = "";
        {
          std::lock_guard lock(*mutex);
          if (!data->empty()) {
            chunk = std::move(data->front());
            data->pop_front();
          }
        }

        if (!chunk) {
          break;
        }

        if (chunk->empty()) {
          std::this_thread::sleep_for(std::chrono::microseconds(100));
          continue;
        }

        if (!record_callback(std::move(*chunk))) {
          break;
        }
      }

      return Error::kOk;
    };
    return record;
  }

  IHttpClient::Headers MakeHeadersFromLabels(const WriteOptions& options) const {
    IHttpClient::Headers headers;
    for (const auto& [key, value] : options.labels) {
      headers.emplace(fmt::format("x-reduct-label-{}", key), value);
    }
    return headers;
  }

  bool SupportsBatchProtocolV2() const {
    auto api_version = client_->ApiVersion();
    return api_version && internal::IsCompatible("1.18", *api_version);
  }

  Result<WriteBatchErrors> ProcessBatch(std::string_view entry_name, BatchCallback callback,
                                        BatchType type) const noexcept {
    Batch batch;
    callback(&batch);

    if (SupportsBatchProtocolV2()) {
      return internal::ProcessBatchV2(*client_, io_path_, entry_name, std::move(batch), type);
    }

    return internal::ProcessBatchV1(*client_, path_, entry_name, std::move(batch), type);
  }

  std::unique_ptr<internal::IHttpClient> client_;
  std::string name_;
  std::string path_;
  std::string io_path_;
  std::thread worker_;

  using Task = std::packaged_task<void()>;
  mutable moodycamel::ConcurrentQueue<Task> task_queue_;
  std::atomic<bool> stop_;
};

std::unique_ptr<IBucket> IBucket::Build(std::string_view server_url, std::string_view name,
                                        const HttpOptions& options) noexcept {
  return std::make_unique<Bucket>(server_url, name, options);
}

// Settings
std::ostream& operator<<(std::ostream& os, const reduct::IBucket::Settings& settings) {
  os << internal::BucketSettingToJsonString(settings).dump();
  return os;
}

std::ostream& operator<<(std::ostream& os, const IBucket::BucketInfo& info) {
  os << fmt::format(
      "<BucketInfo name={}, entry_count={}, size={}, "
      "oldest_record={}, latest_record={}, is_provisioned={}, status={}>",
      info.name, info.entry_count, info.size, info.oldest_record.time_since_epoch().count() / 1000,
      info.latest_record.time_since_epoch().count() / 1000, info.is_provisioned ? "true" : "false",
      info.status == IBucket::Status::kReady ? "READY" : "DELETING");
  return os;
}

std::ostream& operator<<(std::ostream& os, const IBucket::EntryInfo& info) {
  os << fmt::format(
      "<EntryInfo name={}, record_count={}, block_count={}, size={}, oldest_record={}, latest_record={}, status={}>",
      info.name, info.record_count, info.block_count, info.size,
      info.oldest_record.time_since_epoch().count() / 1000,
      info.latest_record.time_since_epoch().count() / 1000,
      info.status == IBucket::Status::kReady ? "READY" : "DELETING");
  return os;
}
}  // namespace reduct
