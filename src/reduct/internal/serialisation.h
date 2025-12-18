// Copyright 2022 Alexey Timin

#ifndef REDUCTCPP_SERIALISATION_H
#define REDUCTCPP_SERIALISATION_H

#include <string>

#include <nlohmann/json.hpp>

#include "reduct/bucket.h"
#include "reduct/client.h"

namespace reduct::internal {

/**
 * @brief Serialize Bucket Setttings to JSON string
 * @return
 */
nlohmann::json BucketSettingToJsonString(const IBucket::Settings& settings);

/**
 * @brief Parse Bucket Settings from JSON string
 * @param json
 * @return
 */
Result<IBucket::Settings> ParseBucketSettings(const nlohmann::json& json);

/**
 * @brief Parse Bucket Info from JSON string
 * @param json
 * @return
 */
Result<IClient::FullTokenInfo> ParseTokenInfo(const nlohmann::json& json);

/**
 * @brief Parse list of replication info from JSON string
 * @param json
 * @return
 */
Result<std::vector<IClient::ReplicationInfo>> ParseReplicationList(const nlohmann::json& data);

/**
 * @brief Convert replication mode to string representation
 * @param mode replication mode
 * @return string value expected by API
 */
std::string ReplicationModeToString(IClient::ReplicationMode mode);

/**
 * @brief Serialize replication settings
 * @param settings to serialize
 * @return json
 */
Result<nlohmann::json> ReplicationSettingsToJsonString(IClient::ReplicationSettings settings);

/**
 * @brief Parse full replication info from JSON string
 * @param data
 * @return
 */
Result<IClient::FullReplicationInfo> ParseFullReplicationInfo(const nlohmann::json& data);

Result<nlohmann::ordered_json> QueryOptionsToJsonString(std::string_view type, std::optional<IBucket::Time> start,
                                                std::optional<IBucket::Time> stop,
                                                const IBucket::QueryOptions& options);

Result<nlohmann::json> QueryLinkOptionsToJsonString(std::string_view bucket, std::string_view entry_name,
                                                    const IBucket::QueryLinkOptions& options);

};  // namespace reduct::internal

#endif  // REDUCTCPP_SERIALISATION_H
