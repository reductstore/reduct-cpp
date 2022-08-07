// Copyright 2022 Alexey Timin

#include "reduct/bucket.h"
#define FMT_HEADER_ONLY 1
#include <fmt/core.h>
#if CONAN
#include <moodycamel/concurrentqueue.h>
#else
#include <concurrentqueue.h>
#endif

#include <nlohmann/json.hpp>

#include <future>
#include <mutex>
#include <queue>
#include <thread>

#include "reduct/internal/http_client.h"
#include "reduct/internal/serialisation.h"

namespace reduct {

using internal::IHttpClient;

class Bucket : public IBucket {
 public:
  Bucket(std::string_view url, std::string_view name, const HttpOptions& options)
      : path_(fmt::format("/b/{}", name)), stop_{} {
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

  ~Bucket() {
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
      auto as_ul = [&info](std::string_view key) { return std::stoul(info.at(key.data()).get<std::string>()); };
      return {
          BucketInfo{
              .name = info.at("name"),
              .entry_count = as_ul("entry_count"),
              .size = as_ul("size"),
              .oldest_record = Time() + std::chrono::microseconds(as_ul("oldest_record")),
              .latest_record = Time() + std::chrono::microseconds(as_ul("latest_record")),
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
        auto as_ul = [&entry](std::string_view key) { return std::stoul(entry.at(key.data()).get<std::string>()); };

        entries[i] = EntryInfo{
            .name = entry.at("name"),
            .record_count = as_ul("record_count"),
            .block_count = as_ul("block_count"),
            .size = as_ul("size"),
            .oldest_record = Time() + std::chrono::microseconds(as_ul("oldest_record")),
            .latest_record = Time() + std::chrono::microseconds(as_ul("latest_record")),
        };
      }

      return {entries, Error::kOk};
    } catch (const std::exception& e) {
      return {{}, Error{.code = -1, .message = e.what()}};
    }
  }

  Error Remove() const noexcept override { return client_->Delete(path_); }

  Error Write(std::string_view entry_name, std::string_view data, std::optional<Time> ts) const noexcept override {
    return Write(entry_name, ts, data.size(), [&data](auto offset, auto size) -> std::pair<bool, std::string> {
      return {true, std::string(data.substr(offset, size))};
    });
  }

  Error Write(std::string_view entry_name, std::optional<Time> ts, size_t content_length,
              WriteCallback callback) const noexcept override {
    if (!ts) {
      ts = Time::clock::now();
    }
    return client_->Post(fmt::format("{}/{}?ts={}", path_, entry_name, ToMicroseconds(*ts)), "application/octet-stream",
                         content_length, std::move(callback));
  }

  Result<std::string> Read(std::string_view entry_name, std::optional<Time> ts) const noexcept override {
    std::string data;
    auto err = Read(entry_name, ts, [&data](auto chunk) {
      data.append(chunk);
      return true;
    });

    if (err) {
      return {{}, err};
    }

    return {std::move(data), Error::kOk};
  }

  Error Read(std::string_view entry_name, std::optional<Time> ts, ReadCallback callback) const noexcept override {
    if (ts) {
      return client_->Get(
          fmt::format("{}/{}?ts={}", path_, entry_name, ToMicroseconds(*ts)), [](auto h) {}, std::move(callback));
    } else {
      return client_->Get(
          fmt::format("{}/{}", path_, entry_name), [](auto h) {}, std::move(callback));
    }
  }

  Result<std::vector<RecordInfo>> List(std::string_view entry_name, Time start, Time stop) const noexcept override {
    auto [body, err] = client_->Get(
        fmt::format("{}/{}/list?start={}&stop={}", path_, entry_name, ToMicroseconds(start), ToMicroseconds(stop)));
    if (err) {
      return {{}, std::move(err)};
    }

    std::vector<RecordInfo> records;
    try {
      auto data = nlohmann::json::parse(body);
      auto json_records = data.at("records");
      records.resize(json_records.size());
      for (int i = 0; i < records.size(); ++i) {
        records[i].timestamp = FromMicroseconds(json_records[i].at("ts"));
        records[i].size = std::stoul(json_records[i].at("size").get<std::string>());
      }
    } catch (const std::exception& ex) {
      return {{}, Error{.code = -1, .message = ex.what()}};
    }

    return {records, Error::kOk};
  }

  Error Query(std::string_view entry_name, std::optional<Time> start, std::optional<Time> stop,
              std::optional<QueryOptions> options, NextRecordCallback callback) const noexcept override {
    auto url = fmt::format("{}/{}/q?", path_, entry_name);

    if (start) {
      url += fmt::format("start={}&", ToMicroseconds(*start));
    }

    if (stop) {
      url += fmt::format("stop={}&", ToMicroseconds(*stop));
    }

    if (options) {
      url += fmt::format("ttl={}", options->ttl.count());
    }

    auto [body, err] = client_->Get(url);
    if (err) {
      return err;
    }

    std::string id;
    try {
      auto data = nlohmann::json::parse(body);
      id = data.at("id");
    } catch (const std::exception& ex) {
      return Error{.code = -1, .message = ex.what()};
    }

    bool last = false;
    while (!last) {
      Record record;
      moodycamel::ConcurrentQueue<std::string> data;
      std::future<void> future;

      err = client_->Get(
          fmt::format("{}/{}?q={}", path_, entry_name, id),
          [&record, &last, &data, &callback, &future, this](IHttpClient::Headers&& headers) {
            last = std::stoi(headers["x-reduct-last"]);
            record.last = last;
            record.timestamp = FromMicroseconds(headers["x-reduct-time"]);
            record.size = std::stoi(headers["content-length"]);

            record.Read = [&](auto record_callback) {
              while (true) {
                std::string chunk;
                if (data.try_dequeue(chunk)) {
                  if (chunk.empty()) {
                    break;
                  }

                  if (!record_callback(std::move(chunk))) {
                    break;
                  }
                } else {
                  std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
              }

              return Error::kOk;
            };

            Task task([&record, &callback, &last] { last = !callback(std::move(record)) || last; });
            future = task.get_future();
            task_queue_.enqueue(std::move(task));
          },
          [&data](auto chunk) {
            data.enqueue(std::string(chunk));
            return true;
          });

      data.enqueue("");
      future.wait();

      if (err) {
        return err;
      }
    }

    return Error::kOk;
  }

 private:
  static int64_t ToMicroseconds(const Time& ts) {
    return std::chrono::duration_cast<std::chrono::microseconds>(ts.time_since_epoch()).count();
  }

  static Time FromMicroseconds(const std::string& ts) { return Time() + std::chrono::microseconds(std::stoul(ts)); }

  std::unique_ptr<internal::IHttpClient> client_;
  std::string path_;
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

std::ostream& operator<<(std::ostream& os, const IBucket::RecordInfo& record) {
  os << fmt::format("<RecordInfo ts={}, size={}>",
                    std::chrono::duration_cast<std::chrono::microseconds>(record.timestamp.time_since_epoch()).count(),
                    record.size);
  return os;
}

std::ostream& operator<<(std::ostream& os, const IBucket::BucketInfo& info) {
  os << fmt::format("<BucketInfo name={}, entry_count={}, size={}, oldest_record={}, latest_record={}>", info.name,
                    info.entry_count, info.size, info.oldest_record.time_since_epoch().count() / 1000,
                    info.latest_record.time_since_epoch().count() / 1000);
  return os;
}

std::ostream& operator<<(std::ostream& os, const IBucket::EntryInfo& info) {
  os << fmt::format("<EntryInfo name={}, record_count={}, block_count={}, size={}, oldest_record={}, latest_record={}>",
                    info.name, info.record_count, info.block_count, info.size,
                    info.oldest_record.time_since_epoch().count() / 1000,
                    info.latest_record.time_since_epoch().count() / 1000);
  return os;
}
}  // namespace reduct
