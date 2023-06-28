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
        };
      }

      return {entries, Error::kOk};
    } catch (const std::exception& e) {
      return {{}, Error{.code = -1, .message = e.what()}};
    }
  }

  Error Remove() const noexcept override { return client_->Delete(path_); }

  Error Write(std::string_view entry_name, std::optional<Time> ts,
              WriteRecordCallback callback) const noexcept override {
    return Write(entry_name, {.timestamp = ts}, std::move(callback));
  }

  Error Write(std::string_view entry_name, const WriteOptions& options,
              WriteRecordCallback callback) const noexcept override {
    WritableRecord record;
    callback(&record);

    const auto time = options.timestamp ? ToMicroseconds(*options.timestamp) : ToMicroseconds(Time::clock::now());
    const auto content_type = options.content_type.empty() ? "application/octet-stream" : options.content_type;

    IHttpClient::Headers headers;
    for (const auto& [key, value] : options.labels) {
      headers.emplace(fmt::format("x-reduct-label-{}", key), value);
    }

    return client_->Post(fmt::format("{}/{}?ts={}", path_, entry_name, time), content_type, record.content_length_,
                         std::move(headers), std::move(record.callback_));
  }

  Error Read(std::string_view entry_name, std::optional<Time> ts, ReadRecordCallback callback) const noexcept override {
    auto path = fmt::format("{}/{}", path_, entry_name);
    if (ts) {
      path.append(fmt::format("?ts={}", ToMicroseconds(*ts)));
    }

    auto record_err = ReadRecord(std::move(path), false, callback);
    return record_err;
  }

  Error Query(std::string_view entry_name, std::optional<Time> start, std::optional<Time> stop, QueryOptions options,
              ReadRecordCallback callback) const noexcept override {
    auto url = fmt::format("{}/{}/q?", path_, entry_name);

    if (start) {
      url += fmt::format("start={}&", ToMicroseconds(*start));
    }

    if (stop) {
      url += fmt::format("stop={}&", ToMicroseconds(*stop));
    }

    if (options.ttl) {
      url += fmt::format("ttl={}&", options.ttl->count());
    }

    if (options.continuous) {
      url += "continuous=true&";
    }

    for (const auto& [key, value] : options.include) {
      url += fmt::format("&include-{}={}", key, value);
    }

    for (const auto& [key, value] : options.exclude) {
      url += fmt::format("&exclude-{}={}", key, value);
    }

    auto [body, err] = client_->Get(url);
    if (err) {
      return err;
    }

    uint64_t id;
    try {
      auto data = nlohmann::json::parse(body);
      id = data.at("id");
    } catch (const std::exception& ex) {
      return Error{.code = -1, .message = ex.what()};
    }

    while (true) {
      bool batched = client_->api_version() >= "1.5";
      auto [stopped, record_err] =
          ReadRecord(fmt::format("{}/{}{}?q={}", path_, entry_name, batched ? "/batch" : "", id), batched, callback);

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

 private:
  Result<bool> ReadRecord(std::string&& path, bool batched, const ReadRecordCallback& callback) const noexcept {
    std::deque<std::optional<std::string>> data;
    std::mutex data_mutex;
    std::future<void> future;
    bool stopped = false;

    auto err = client_->Get(
        path,
        [&stopped, &data, &data_mutex, &callback, &future, batched, this](IHttpClient::Headers&& headers) {
          std::vector<ReadableRecord> records;
          if (batched) {
            records = ParseAndBuildBatchedRecords(&data, &data_mutex, std::move(headers));
          } else {
            records.push_back(ParseAndBuildSingleRecord(&data, &data_mutex, std::move(headers)));
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
        },
        [&data, &data_mutex](auto chunk) {
          {
            std::lock_guard lock(data_mutex);
            data.emplace_back(std::string(chunk));
          }
          return true;
        });

    if (!err) {
      {
        std::lock_guard lock(data_mutex);
        data.emplace_back(std::nullopt);
      }
      future.wait();
    }

    return {stopped, err};
  }

  static ReadableRecord ParseAndBuildSingleRecord(std::deque<std::optional<std::string>>* data, std::mutex* mutex,
                                                  IHttpClient::Headers&& headers) {
    ReadableRecord record;

    record.timestamp = FromMicroseconds(headers["x-reduct-time"]);
    record.size = std::stoi(headers["content-length"]);
    record.content_type = headers["content-type"];

    for (const auto& [key, value] : headers) {
      if (key.starts_with("x-reduct-label-")) {
        record.labels.emplace(key.substr(15), value);
      }
    }

    record.Read = [data, mutex](auto record_callback) {
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

  static std::vector<ReadableRecord> ParseAndBuildBatchedRecords(std::deque<std::optional<std::string>>* data,
                                                                 std::mutex* mutex, IHttpClient::Headers&& headers) {
    auto parse_csv = [](const std::string& csv) {
      std::vector<std::string> items;
      std::string escaped, item;
      std::stringstream ss(csv);

      while (std::getline(ss, item, ',')) {
        if (item.starts_with("\"") && escaped.empty()) {
          escaped = item.substr(1);
        }

        if (!escaped.empty()) {
          if (item.ends_with("\"")) {
            escaped = escaped.substr(0, escaped.size() - 2);
            items.push_back(escaped);
            escaped = "";
          } else {
            escaped += item;
          }
        } else {
          items.push_back(item);
        }
      }

      auto size = std::stoll(items[0]);
      // eslint-disable-next-line prefer-destructuring
      auto content_type = items[1];
      LabelMap labels = {};

      for (auto i = 2; i < items.size(); i++) {
        auto pos = items[i].find('=');
        if (pos == std::string::npos) {
          continue;
        }

        auto key = items[i].substr(0, pos);
        labels[key] = items[i].substr(pos + 1);
      }

      return std::tuple{
          size,
          content_type,
          labels,
      };
    };

    std::vector<ReadableRecord> records;
    size_t total_records = std::count_if(headers.begin(), headers.end(),
                                         [](const auto& header) { return header.first.starts_with("x-reduct-time-"); });

    std::map<std::string , std::string> ordered_headers(headers.begin(), headers.end());
    for (auto header = ordered_headers.begin(); header != ordered_headers.end(); ++header) {
      if (!header->first.starts_with("x-reduct-time-")) {
        continue;
      }

      auto [size, content_type, labels] = parse_csv(header->second);

      ReadableRecord record;
      record.timestamp = FromMicroseconds(header->first.substr(14));
      record.size = size;
      record.content_type = content_type;
      record.labels = labels;
      record.Read = [data, mutex, size](auto record_callback) {
        size_t total = 0;
        while (true) {
          std::optional<std::string> chunk = "";
          {
            std::lock_guard lock(*mutex);
            if (!data->empty()) {
              chunk = std::move(data->front());
              data->pop_front();

              if (chunk->size() > size - total) {
                auto tmp = chunk->substr(0, size - total);
                data->push_front(chunk->substr(size - total));
                chunk = std::move(tmp);
              }
            }
          }

          if (!chunk) {
            break;
          }

          if (chunk->empty()) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            continue;
          }

          total += chunk->size();
          if (!record_callback(std::move(*chunk))) {
            break;
          }

          if (total >= size) {
            break;
          }
        }

        return Error::kOk;
      };

      record.last = (records.size() == total_records - 1 && headers["x-reduct-last"] == "true");
      records.push_back(std::move(record));
    }

    return records;
  }

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
