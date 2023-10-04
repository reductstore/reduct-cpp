// Copyright 2022 Alexey Timin

#include "reduct/internal/serialisation.h"

#include <date/date.h>

namespace reduct::internal {

nlohmann::json BucketSettingToJsonString(const IBucket::Settings& settings) noexcept {
  nlohmann::json data;
  const auto& [max_block_size, quota_type, quota_size, max_record_size] = settings;
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

  if (max_record_size) {
    data["max_block_records"] = *max_record_size;
  }

  return data;
}

Result<IBucket::Settings> ParseBucketSettings(const nlohmann::json& json) noexcept {
  IBucket::Settings settings;
  try {
    if (json.contains("max_block_size")) {
      settings.max_block_size = json["max_block_size"];
    }

    if (json.contains("quota_type")) {
      if (json["quota_type"] == "NONE") {
        settings.quota_type = IBucket::QuotaType::kNone;
      } else if (json["quota_type"] == "FIFO") {
        settings.quota_type = IBucket::QuotaType::kFifo;
      }
    }

    if (json.contains("quota_size")) {
      settings.quota_size = json["quota_size"];
    }

    if (json.contains("max_block_records")) {
      settings.max_block_records = json["max_block_records"];
    }
  } catch (const std::exception& ex) {
    return {{}, Error{.code = -1, .message = ex.what()}};
  }
  return {settings, Error::kOk};
}

Result<IClient::FullTokenInfo> ParseTokenInfo(const nlohmann::json& json) noexcept {
  IClient::Time created_at;
  std::istringstream(json.at("created_at").get<std::string>()) >> date::parse("%FT%TZ", created_at);

  return {
      IClient::FullTokenInfo{
          .name = json.at("name"),
          .created_at = created_at,
          .is_provisioned = json.value("is_provisioned", false),
          .permissions = {.full_access = json.at("permissions").at("full_access"),
                          .read = json.at("permissions").at("read"),
                          .write = json.at("permissions").at("write")},
      },
      Error::kOk,
  };
}
}  // namespace reduct::internal
