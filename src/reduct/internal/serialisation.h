// Copyright 2022 Alexey Timin

#ifndef REDUCTCPP_SERIALISATION_H
#define REDUCTCPP_SERIALISATION_H

#include <nlohmann/json.hpp>

#include "reduct/bucket.h"

namespace reduct::internal {

/**
 * @brief Serialize Bucket Setttings to JSON string
 * @return
 */
nlohmann::json BucketSettingToJsonString(const IBucket::Settings& settings) noexcept;

/**
 * @brief Parse Bucket Settings from JSON string
 * @param json
 * @return
 */
Result<IBucket::Settings> ParseBucketSettings(const nlohmann::json& json) noexcept;
};  // namespace reduct::internal

#endif  // REDUCTCPP_SERIALISATION_H
