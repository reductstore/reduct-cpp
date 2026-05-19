// Copyright 2022-2025 ReductSoftware UG

#include "reduct/internal/serialisation.h"

#include <stdexcept>

#include "time_parse.h"

namespace reduct::internal {

namespace {

IClient::ReplicationMode ParseReplicationMode(const nlohmann::json& mode_json) {
  if (mode_json.is_null()) {
    return IClient::ReplicationMode::kEnabled;
  }

  const auto mode = mode_json.get<std::string>();
  if (mode == "enabled") {
    return IClient::ReplicationMode::kEnabled;
  }

  if (mode == "paused") {
    return IClient::ReplicationMode::kPaused;
  }

  if (mode == "disabled") {
    return IClient::ReplicationMode::kDisabled;
  }

  throw std::invalid_argument("Invalid replication mode: " + mode);
}

IClient::LifecycleMode ParseLifecycleMode(const nlohmann::json& mode_json) {
  if (mode_json.is_null()) {
    return IClient::LifecycleMode::kEnabled;
  }

  const auto mode = mode_json.get<std::string>();
  if (mode == "enabled") {
    return IClient::LifecycleMode::kEnabled;
  }

  if (mode == "disabled") {
    return IClient::LifecycleMode::kDisabled;
  }

  if (mode == "dry_run") {
    return IClient::LifecycleMode::kDryRun;
  }

  throw std::invalid_argument("Invalid lifecycle mode: " + mode);
}

IClient::LifecycleType ParseLifecycleType(const nlohmann::json& type_json) {
  if (type_json.is_null()) {
    return IClient::LifecycleType::kDelete;
  }

  const auto type = type_json.get<std::string>();
  if (type == "delete") {
    return IClient::LifecycleType::kDelete;
  }

  throw std::invalid_argument("Invalid lifecycle type: " + type);
}

}  // namespace

std::string ReplicationModeToString(IClient::ReplicationMode mode) {
  switch (mode) {
    case IClient::ReplicationMode::kEnabled:
      return "enabled";
    case IClient::ReplicationMode::kPaused:
      return "paused";
    case IClient::ReplicationMode::kDisabled:
      return "disabled";
  }

  throw std::invalid_argument("Invalid replication mode");
}

std::string LifecycleModeToString(IClient::LifecycleMode mode) {
  switch (mode) {
    case IClient::LifecycleMode::kEnabled:
      return "enabled";
    case IClient::LifecycleMode::kDisabled:
      return "disabled";
    case IClient::LifecycleMode::kDryRun:
      return "dry_run";
  }

  throw std::invalid_argument("Invalid lifecycle mode");
}

std::string LifecycleTypeToString(IClient::LifecycleType type) {
  switch (type) {
    case IClient::LifecycleType::kDelete:
      return "delete";
  }

  throw std::invalid_argument("Invalid lifecycle type");
}

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
  IClient::Time created_at = parse_iso8601_utc(json.at("created_at").get<std::string>());

  std::optional<IClient::Time> expires_at;
  if (json.contains("expires_at") && !json.at("expires_at").is_null()) {
    expires_at = parse_iso8601_utc(json.at("expires_at").get<std::string>());
  }

  std::optional<IClient::Time> last_access;
  if (json.contains("last_access") && !json.at("last_access").is_null()) {
    last_access = parse_iso8601_utc(json.at("last_access").get<std::string>());
  }

  std::optional<uint64_t> ttl;
  if (json.contains("ttl") && !json.at("ttl").is_null()) {
    ttl = json.at("ttl").get<uint64_t>();
  }

  return {
      IClient::FullTokenInfo{
          .name = json.at("name"),
          .created_at = created_at,
          .is_provisioned = json.value("is_provisioned", false),
          .expires_at = expires_at,
          .ttl = ttl,
          .last_access = last_access,
          .ip_allowlist = json.value("ip_allowlist", std::vector<std::string>{}),
          .is_expired = json.value("is_expired", false),
          .permissions = {.full_access = json.at("permissions").at("full_access"),
                          .read = json.at("permissions").at("read"),
                          .write = json.at("permissions").at("write")},
      },
      Error::kOk,
  };
}

Result<std::vector<IClient::ReplicationInfo>> ParseReplicationList(const nlohmann::json& data) {
  std::vector<IClient::ReplicationInfo> replication_list;

  try {
    auto json_replications = data.at("replications");
    replication_list.reserve(json_replications.size());
    for (const auto& replication : json_replications) {
      replication_list.push_back(IClient::ReplicationInfo{
          .name = replication.at("name"),
          .mode = replication.contains("mode") ? ParseReplicationMode(replication.at("mode"))
                                               : IClient::ReplicationMode::kEnabled,
          .is_active = replication.at("is_active"),
          .is_provisioned = replication.at("is_provisioned"),
          .pending_records = replication.at("pending_records"),
      });
    }
  } catch (const std::exception& ex) {
    return {{}, Error{.code = -1, .message = ex.what()}};
  }

  return {replication_list, Error::kOk};
}

Result<nlohmann::json> ReplicationSettingsToJsonString(IClient::ReplicationSettings settings) {
  try {
    nlohmann::json json_data;
    json_data["src_bucket"] = settings.src_bucket;
    json_data["dst_bucket"] = settings.dst_bucket;
    json_data["dst_host"] = settings.dst_host;
    if (settings.dst_token) {
      json_data["dst_token"] = *settings.dst_token;
    }
    json_data["entries"] = settings.entries;
    json_data["mode"] = ReplicationModeToString(settings.mode);

    if (settings.when) {
      try {
        json_data["when"] = nlohmann::json::parse(*settings.when);
      } catch (const std::exception& ex) {
        return {{}, Error{.code = -1, .message = ex.what()}};
      }
    }

    return {json_data, Error::kOk};
  } catch (const std::exception& ex) {
    return {{}, Error{.code = -1, .message = ex.what()}};
  }
}

Result<IClient::FullReplicationInfo> ParseFullReplicationInfo(const nlohmann::json& data) {
  IClient::FullReplicationInfo info;
  try {
    info.info = IClient::ReplicationInfo{
        .name = data.at("info").at("name"),
        .mode = data.at("info").contains("mode") ? ParseReplicationMode(data.at("info").at("mode"))
                                                 : IClient::ReplicationMode::kEnabled,
        .is_active = data.at("info").at("is_active"),
        .is_provisioned = data.at("info").at("is_provisioned"),
        .pending_records = data.at("info").at("pending_records"),
    };

    const auto& settings = data.at("settings");
    info.settings = IClient::ReplicationSettings{
        .src_bucket = settings.at("src_bucket"),
        .dst_bucket = settings.at("dst_bucket"),
        .dst_host = settings.at("dst_host"),
        .entries = settings.at("entries"),
        .mode =
            settings.contains("mode") ? ParseReplicationMode(settings.at("mode")) : IClient::ReplicationMode::kEnabled,
    };

    if (settings.contains("dst_token") && !settings.at("dst_token").is_null()) {
      info.settings.dst_token = settings.at("dst_token");
    }

    if (settings.contains("when") && !settings.at("when").is_null()) {
      info.settings.when = settings.at("when").dump();
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

Result<std::vector<IClient::LifecycleInfo>> ParseLifecycleList(const nlohmann::json& data) {
  std::vector<IClient::LifecycleInfo> lifecycle_list;

  try {
    auto json_lifecycles = data.at("lifecycles");
    lifecycle_list.reserve(json_lifecycles.size());
    for (const auto& lifecycle : json_lifecycles) {
      lifecycle_list.push_back(IClient::LifecycleInfo{
          .name = lifecycle.at("name"),
          .mode = lifecycle.contains("mode") ? ParseLifecycleMode(lifecycle.at("mode"))
                                              : IClient::LifecycleMode::kEnabled,
          .is_provisioned = lifecycle.at("is_provisioned"),
          .is_running = lifecycle.at("is_running"),
      });
    }
  } catch (const std::exception& ex) {
    return {{}, Error{.code = -1, .message = ex.what()}};
  }

  return {lifecycle_list, Error::kOk};
}

Result<nlohmann::json> LifecycleSettingsToJsonString(IClient::LifecycleSettings settings) {
  try {
    nlohmann::json json_data;
    json_data["type"] = LifecycleTypeToString(settings.type);
    json_data["bucket"] = settings.bucket;
    json_data["entries"] = settings.entries;
    json_data["max_age"] = settings.max_age;
    if (settings.interval) {
      json_data["interval"] = *settings.interval;
    }
    json_data["mode"] = LifecycleModeToString(settings.mode);

    if (settings.when) {
      try {
        json_data["when"] = nlohmann::json::parse(*settings.when);
      } catch (const std::exception& ex) {
        return {{}, Error{.code = -1, .message = ex.what()}};
      }
    }

    return {json_data, Error::kOk};
  } catch (const std::exception& ex) {
    return {{}, Error{.code = -1, .message = ex.what()}};
  }
}

Result<IClient::FullLifecycleInfo> ParseFullLifecycleInfo(const nlohmann::json& data) {
  IClient::FullLifecycleInfo info;
  try {
    info.info = IClient::LifecycleInfo{
        .name = data.at("info").at("name"),
        .mode = data.at("info").contains("mode") ? ParseLifecycleMode(data.at("info").at("mode"))
                                                   : IClient::LifecycleMode::kEnabled,
        .is_provisioned = data.at("info").at("is_provisioned"),
        .is_running = data.at("info").at("is_running"),
    };

    const auto& settings = data.at("settings");
    info.settings = IClient::LifecycleSettings{
        .type = settings.contains("type") ? ParseLifecycleType(settings.at("type")) : IClient::LifecycleType::kDelete,
        .bucket = settings.at("bucket"),
        .entries = settings.at("entries"),
        .max_age = settings.at("max_age"),
        .mode = settings.contains("mode") ? ParseLifecycleMode(settings.at("mode"))
                                           : IClient::LifecycleMode::kEnabled,
    };

    if (settings.contains("interval") && !settings.at("interval").is_null()) {
      info.settings.interval = settings.at("interval");
    }

    if (settings.contains("when") && !settings.at("when").is_null()) {
      info.settings.when = settings.at("when").dump();
    }
  } catch (const std::exception& ex) {
    return {{}, Error{.code = -1, .message = ex.what()}};
  }

  return {info, Error::kOk};
}

Result<nlohmann::ordered_json> QueryOptionsToJsonString(std::string_view type, const std::vector<std::string>& entries,
                                                        std::optional<IBucket::Time> start,
                                                        std::optional<IBucket::Time> stop,
                                                        const IBucket::QueryOptions& options) {
  nlohmann::ordered_json json_data;
  json_data["query_type"] = type;

  json_data["entries"] = nlohmann::ordered_json::array();
  for (const auto& entry : entries) {
    json_data["entries"].push_back(entry);
  }

  if (start) {
    json_data["start"] = std::chrono::duration_cast<std::chrono::microseconds>(start->time_since_epoch()).count();
  }

  if (stop) {
    json_data["stop"] = std::chrono::duration_cast<std::chrono::microseconds>(stop->time_since_epoch()).count();
  }

  if (options.ttl) {
    json_data["ttl"] = options.ttl->count() / 1000;
  }

  if (options.continuous) {
    json_data["continuous"] = true;
  }

  if (options.when) {
    try {
      json_data["when"] = nlohmann::ordered_json::parse(*options.when);
    } catch (const std::exception& ex) {
      return {{}, Error{.code = -1, .message = ex.what()}};
    }
  }

  if (options.strict) {
    json_data["strict"] = *options.strict;
  }

  if (options.ext) {
    try {
      json_data["ext"] = nlohmann::ordered_json::parse(*options.ext);
    } catch (const std::exception& ex) {
      return {{}, Error{.code = -1, .message = ex.what()}};
    }
  }

  return {json_data, Error::kOk};
}

Result<nlohmann::ordered_json> QueryLinkOptionsToJsonString(std::string_view bucket,
                                                            const std::vector<std::string>& entries,
                                                            const IBucket::QueryLinkOptions& options) {
  nlohmann::ordered_json json_data;

  json_data["bucket"] = bucket;
  if (!options.record_entry && !options.record_timestamp) {
    json_data["index"] = options.record_index;
  }
  if (options.record_entry) {
    json_data["record_entry"] = *options.record_entry;
  }
  if (options.record_timestamp) {
    json_data["record_timestamp"] =
        std::chrono::duration_cast<std::chrono::microseconds>(options.record_timestamp->time_since_epoch()).count();
  }
  json_data["entry"] = entries.at(0);
  auto [query_json, query_err] =
      QueryOptionsToJsonString("QUERY", entries, options.start, options.stop, options.query_options);
  if (query_err) {
    return {{}, std::move(query_err)};
  }

  json_data["query"] = std::move(query_json);

  if (options.expire_at) {
    json_data["expire_at"] =
        std::chrono::duration_cast<std::chrono::seconds>(options.expire_at->time_since_epoch()).count();
  } else {
    // Default expire time is 24 hours
    json_data["expire_at"] = std::chrono::duration_cast<std::chrono::seconds>(
                                 (IBucket::Time::clock::now() + std::chrono::hours(24)).time_since_epoch())
                                 .count();
  }

  if (options.file_name) {
    json_data["file_name"] = *options.file_name;
  }

  if (options.base_url) {
    json_data["base_url"] = *options.base_url;
  }

  return {json_data, Error::kOk};
}

IBucket::Status ParseStatus(const nlohmann::json& json) {
  if (json.contains("status")) {
    const auto status = json.at("status").get<std::string>();
    if (status == "DELETING") {
      return IBucket::Status::kDeleting;
    }
  }
  return IBucket::Status::kReady;
}

}  // namespace reduct::internal
