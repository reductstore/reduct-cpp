//
// Created by atimin on 02.02.22.
//

#include "reduct/bucket.h"

#include <nlohmann/json.hpp>

namespace reduct {
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

}  // namespace reduct