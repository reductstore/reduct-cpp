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
      case IBucket::QuotaType::kHard:
        data["quota_type"] = "HARD";
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
      } else if (json["quota_type"] == "HARD") {
        settings.quota_type = IBucket::QuotaType::kHard;
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
  if (settings.each_s) {
    json_data["each_s"] = *settings.each_s;
  }

  if (settings.each_n) {
    json_data["each_n"] = *settings.each_n;
  }

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

    const auto& settings = data.at("settings");
    info.settings = IClient::ReplicationSettings{
        .src_bucket = settings.at("src_bucket"),
        .dst_bucket = settings.at("dst_bucket"),
        .dst_host = settings.at("dst_host"),
        .dst_token = settings.at("dst_token"),
        .entries = settings.at("entries"),
        .include = settings.at("include"),
        .exclude = settings.at("exclude"),
    };

    if (settings.contains("each_s") && !settings.at("each_s").is_null()) {
      info.settings.each_s = settings.at("each_s");
    }

    if (settings.contains("each_n") && !settings.at("each_n").is_null()) {
      info.settings.each_n = settings.at("each_n");
    }

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

Result<nlohmann::json> QueryOptionsToJsonString(std::string_view type, const IBucket::QueryOptions& options) {
  nlohmann::json json_data;
  json_data["query_type"] = type;

  for (const auto& [key, value] : options.include) {
    json_data["include"][key] = value;
  }

  for (const auto& [key, value] : options.exclude) {
    json_data["exclude"][key] = value;
  }

  if (options.each_s) {
    json_data["each_s"] = *options.each_s;
  }

  if (options.each_n) {
    json_data["each_n"] = *options.each_n;
  }

  if (options.limit) {
    json_data["limit"] = *options.limit;
  }

  if (options.ttl) {
    json_data["ttl"] = options.ttl->count() / 1000;
  }

  if (options.continuous) {
    json_data["continuous"] = true;
  }

  if (options.when) {
    try {
      json_data["when"] = nlohmann::json::parse(*options.when);
    } catch (const std::exception& ex) {
      return {{}, Error{.code = -1, .message = ex.what()}};
    }
  }

  if (options.strict) {
    json_data["strict"] = *options.strict;
  }

  return  {json_data, Error::kOk};
}

}  // namespace reduct::internal
