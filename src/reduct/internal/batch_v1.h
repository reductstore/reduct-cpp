// Copyright 2026 ReductSoftware UG
#ifndef REDUCT_CPP_BATCH_V1_H
#define REDUCT_CPP_BATCH_V1_H

#include <chrono>
#include <deque>
#include <mutex>
#include <optional>
#include <string_view>
#include <string>
#include <vector>

#include "reduct/bucket.h"
#include "reduct/internal/http_client.h"
#include "reduct/result.h"

namespace reduct::internal {

enum class BatchType { kWrite, kUpdate, kRemove };

int64_t ToMicroseconds(const IBucket::Time& ts);
IBucket::Time FromMicroseconds(const std::string& ts);

std::string RecordEntry(const IBucket::Batch::Record& record, std::string_view default_entry);
std::string FormatLabels(const IBucket::LabelMap& labels);
std::vector<size_t> SortRecords(const IBucket::Batch& batch, const std::string& default_entry,
                                bool sort_by_entry);

std::vector<IBucket::ReadableRecord> ParseAndBuildBatchedRecordsV1(
    std::deque<std::optional<std::string>>* data, std::mutex* mutex, bool head, IHttpClient::Headers&& headers);

Result<IBucket::WriteBatchErrors> ProcessBatchV1(IHttpClient& client, std::string_view bucket_path,
                                                 std::string_view entry_name, IBucket::Batch batch, BatchType type);

}  // namespace reduct::internal

#endif  // REDUCT_CPP_BATCH_V1_H
