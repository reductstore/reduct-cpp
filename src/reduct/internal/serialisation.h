// Copyright 2022 Alexey Timin

#ifndef REDUCTCPP_SERIALISATION_H
#define REDUCTCPP_SERIALISATION_H

#include <nlohmann/json.hpp>

#include "reduct/client.h"
#include "reduct/bucket.h"

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
 * @brief Serialize replication settings
 * @param settings to serialize
 * @return json
 */
nlohmann::json ReplicationSettingsToJsonString(IClient::ReplicationSettings settings);

/**
 * @brief Parse full replication info from JSON string
 * @param data
 * @return
 */
Result<IClient::FullReplicationInfo> ParseFullReplicationInfo(const nlohmann::json& data);

};  // namespace reduct::internal

#endif  // REDUCTCPP_SERIALISATION_H
