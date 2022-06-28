// Copyright 2022 Alexey Timin

#include "reduct/bucket.h"
#define FMT_HEADER_ONLY 1
#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include "reduct/internal/http_client.h"
#include "reduct/internal/serialisation.h"

namespace reduct {

class Bucket : public IBucket {
 public:
  Bucket(std::string_view url, std::string_view name, const HttpOptions& options) : path_(fmt::format("/b/{}", name)) {
    client_ = internal::IHttpClient::Build(url, options);
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
    if (!ts) {
      ts = Time::clock::now();
    }
    return client_->Post(fmt::format("{}/{}?ts={}", path_, entry_name, ToMicroseconds(*ts)), data.data(),
                         "application/octet-stream");
  }

  Error Write(std::string_view entry_name, std::optional<Time> ts, size_t content_length, WriteCallback callback) const noexcept override {
    if (!ts) {
      ts = Time::clock::now();
    }
    return client_->Post(fmt::format("{}/{}?ts={}", path_, entry_name, ToMicroseconds(*ts)),
                         "application/octet-stream", content_length, std::move(callback));
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
      return client_->Get(fmt::format("{}/{}?ts={}", path_, entry_name, ToMicroseconds(*ts)), std::move(callback));
    } else {
      return client_->Get(fmt::format("{}/{}", path_, entry_name), std::move(callback));
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

 private:
  static int64_t ToMicroseconds(const Time& ts) {
    return std::chrono::duration_cast<std::chrono::microseconds>(ts.time_since_epoch()).count();
  }

  static Time FromMicroseconds(const std::string& ts) { return Time() + std::chrono::microseconds(std::stoul(ts)); }

  std::unique_ptr<internal::IHttpClient> client_;
  std::string path_;
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
