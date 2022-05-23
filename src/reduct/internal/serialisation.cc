// Copyright 2022 Alexey Timin

#include "reduct/internal/serialisation.h"

namespace reduct::internal {

nlohmann::json BucketSettingToJsonString(const IBucket::Settings& settings) noexcept {
  nlohmann::json data;
  const auto& [max_block_size, quota_type, quota_size] = settings;
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

  return data;
}

Result<IBucket::Settings> ParseBucketSettings(const nlohmann::json& json) noexcept {
  IBucket::Settings settings;
  try {
    if (json.contains("max_block_size")) {
      settings.max_block_size = std::stoul(json["max_block_size"].get<std::string>());
    }

    if (json.contains("quota_type")) {
      if (json["quota_type"] == "NONE") {
        settings.quota_type = IBucket::QuotaType::kNone;
      } else if (json["quota_type"] == "FIFO") {
        settings.quota_type = IBucket::QuotaType::kFifo;
      }
    }

    if (json.contains("quota_size")) {
      settings.quota_size = std::stoul(json["quota_size"].get<std::string>());
    }
  } catch (const std::exception& ex) {
    return {{}, Error{.code = -1, .message = ex.what()}};
  }
  return {settings, Error::kOk};
}
}  // namespace reduct::internal