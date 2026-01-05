// Copyright 2026 ReductSoftware UG
#ifndef REDUCT_CPP_BATCH_V2_H
#define REDUCT_CPP_BATCH_V2_H

#include <deque>
#include <mutex>
#include <optional>
#include <string_view>
#include <vector>

#include "reduct/internal/batch_v1.h"

namespace reduct::internal {

std::vector<IBucket::ReadableRecord> ParseAndBuildBatchedRecordsV2(
    std::deque<std::optional<std::string>>* data, std::mutex* mutex, bool head, IHttpClient::Headers&& headers);

Result<IBucket::WriteBatchErrors> ProcessBatchV2(IHttpClient& client, std::string_view io_path,
                                                 std::string_view entry_name, IBucket::Batch batch, BatchType type);

}  // namespace reduct::internal

#endif  // REDUCT_CPP_BATCH_V2_H
