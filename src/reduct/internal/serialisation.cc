// Copyright 2022 Alexey Timin

#include "reduct/internal/serialisation.h"

#ifdef REDUCT_CPP_USE_STD_CHRONO
#include <chrono>
#else
#include <date/date.h>
#endif

namespace reduct::internal {

nlohmann::json BucketSettingToJsonString(const IBucket::Settings& settings) {
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

Result<IBucket::Settings> ParseBucketSettings(const nlohmann::json& json) {
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

Result<IClient::FullTokenInfo> ParseTokenInfo(const nlohmann::json& json) {
  IClient::Time created_at;
#ifdef REDUCT_CPP_USE_STD_CHRONO
  std::istringstream(json.at("created_at").get<std::string>()) >> std::chrono::parse("%FT%TZ", created_at);
#else
  std::istringstream(json.at("created_at").get<std::string>()) >> date::parse("%FT%TZ", created_at);
#endif

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

Result<std::vector<IClient::ReplicationInfo>> ParseReplicationList(const nlohmann::json& data) {
  std::vector<IClient::ReplicationInfo> replication_list;

  auto json_replications = data.at("replications");
  replication_list.reserve(json_replications.size());
  for (const auto& replication : json_replications) {
    replication_list.push_back(IClient::ReplicationInfo{
        .name = replication.at("name"),
        .is_active = replication.at("is_active"),
        .is_provisioned = replication.at("is_provisioned"),
        .pending_records = replication.at("pending_records"),
    });
  }

  return {replication_list, Error::kOk};
}

nlohmann::json ReplicationSettingsToJsonString(IClient::ReplicationSettings settings) {
  nlohmann::json json_data;
  json_data["src_bucket"] = settings.src_bucket;
  json_data["dst_bucket"] = settings.dst_bucket;
  json_data["dst_host"] = settings.dst_host;
  json_data["dst_token"] = settings.dst_token;
  json_data["entries"] = settings.entries;
  json_data["include"] = settings.include;
  json_data["exclude"] = settings.exclude;

  return json_data;
}

Result<IClient::FullReplicationInfo> ParseFullReplicationInfo(const nlohmann::json& data) {
  IClient::FullReplicationInfo info;
  try {
    info.info = IClient::ReplicationInfo{
        .name = data.at("info").at("name"),
        .is_active = data.at("info").at("is_active"),
        .is_provisioned = data.at("info").at("is_provisioned"),
        .pending_records = data.at("info").at("pending_records"),
    };

    info.settings = IClient::ReplicationSettings{
        .src_bucket = data.at("settings").at("src_bucket"),
        .dst_bucket = data.at("settings").at("dst_bucket"),
        .dst_host = data.at("settings").at("dst_host"),
        .dst_token = data.at("settings").at("dst_token"),
        .entries = data.at("settings").at("entries"),
        .include = data.at("settings").at("include"),
        .exclude = data.at("settings").at("exclude"),
    };

    auto diagnostics = data.at("diagnostics");
    info.diagnostics = Diagnostics{.hourly = DiagnosticsItem{
                                       .ok = diagnostics.at("hourly").at("ok"),
                                       .errored = diagnostics.at("hourly").at("errored"),
                                       .errors = {},
                                   }};

    for (const auto& [key, value] : diagnostics.at("hourly").at("errors").items()) {
      info.diagnostics.hourly.errors[std::stoi(key)] = DiagnosticsError{
          .count = value.at("count"),
          .last_message = value.at("last_message"),
      };
    }
  } catch (const std::exception& ex) {
    return {{}, Error{.code = -1, .message = ex.what()}};
  }

  return {info, Error::kOk};
}

}  // namespace reduct::internal
