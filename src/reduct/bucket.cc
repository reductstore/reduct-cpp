// Copyright 2022 Alexey Timin

#include "reduct/bucket.h"
#define FMT_HEADER_ONLY 1
#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include "reduct/internal/http_client.h"

namespace reduct {

class Bucket : public IBucket {
 public:
  Bucket(std::string_view url, std::string_view name) : path_(fmt::format("/b/{}", name)) {
    client_ = internal::IHttpClient::Build(url);
  }

  Result<Settings> GetSettings() const noexcept override {
    auto [body, err] = client_->Get(path_);
    if (err) {
      return {{}, std::move(err)};
    }
    return Settings::Parse(body);
  }

  Error UpdateSettings(const Settings& settings) const noexcept override {
    auto [current_setting, get_err] = GetSettings();
    if (get_err) {
      return get_err;
    }

    return client_->Put(path_, current_setting.ToJsonString());
  }

  Error Remove() const noexcept override { return client_->Delete(path_); }

  Error Write(std::string_view entry_name, std::string_view data, Time ts) const noexcept override {
    return client_->Post(fmt::format("{}/{}?ts={}", path_, entry_name, ToMicroseconds(ts)), data.data(),
                         "application/octet-stream");
  }

  Result<std::string> Read(std::string_view entry_name, Time ts) const noexcept override {
    return client_->Get(fmt::format("{}/{}?ts={}", path_, entry_name, ToMicroseconds(ts)));
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
        records[i].size = json_records[i].at("size");
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

  static Time FromMicroseconds(int64_t ts) { return Time() + std::chrono::microseconds(ts); }

  std::unique_ptr<internal::IHttpClient> client_;
  std::string path_;
};

std::unique_ptr<IBucket> IBucket::Build(std::string_view server_url, std::string_view name) noexcept {
  return std::make_unique<Bucket>(server_url, name);
}

// Settings
std::ostream& operator<<(std::ostream& os, const reduct::IBucket::Settings& settings) {
  os << settings.ToJsonString();
  return os;
}

std::string IBucket::Settings::ToJsonString() const noexcept {
  nlohmann::json data;
  if (max_block_size) {
    data["max_block_size"] = *max_block_size;
  }

  if (quota_type) {
    switch (*quota_type) {
      case IBucket::QuotaType::kNone:
        data["quota_type"] = "NONE";
        break;
      case IBucket::QuotaType::kFifo:
        data["quota_type"] = "FIFO";
        break;
    }
  }

  if (quota_size) {
    data["quota_size"] = *quota_size;
  }

  return data.dump();
}

Result<IBucket::Settings> IBucket::Settings::Parse(std::string_view json) noexcept {
  IBucket::Settings settings;
  try {
    auto data = nlohmann::json::parse(json);
    if (data.contains("max_block_size")) {
      settings.max_block_size = data["max_block_size"];
    }

    if (data.contains("quota_type")) {
      if (data["quota_type"] == "NONE") {
        settings.quota_type = IBucket::QuotaType::kNone;
      } else if (data["quota_type"] == "FIFO") {
        settings.quota_type = IBucket::QuotaType::kFifo;
      }
    }

    if (data.contains("quota_size")) {
      settings.quota_size = data["quota_size"];
    }
  } catch (const std::exception& ex) {
    return {{}, Error{.code = -1, .message = ex.what()}};
  }
  return {settings, Error::kOk};
}

std::ostream& operator<<(std::ostream& os, const IBucket::RecordInfo& record) {
  os << fmt::format("<RecordInfo ts={}, size={}",
                    std::chrono::duration_cast<std::chrono::microseconds>(record.timestamp.time_since_epoch()).count(),
                    record.size);
  return os;
}
}  // namespace reduct
