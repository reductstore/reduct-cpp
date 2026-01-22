// Copyright 2026 ReductSoftware UG
#ifndef REDUCT_CPP_HEADERS_H
#define REDUCT_CPP_HEADERS_H

#include <string_view>

namespace reduct::internal {

constexpr std::string_view kHeaderPrefix = "x-reduct-";
constexpr std::string_view kHeaderError = "x-reduct-error";
constexpr std::string_view kHeaderErrorPrefix = "x-reduct-error-";
constexpr std::string_view kHeaderApi = "x-reduct-api";
constexpr std::string_view kHeaderQueryId = "x-reduct-query-id";
constexpr std::string_view kHeaderTime = "x-reduct-time";
constexpr std::string_view kHeaderTimePrefix = "x-reduct-time-";
constexpr std::string_view kHeaderLast = "x-reduct-last";
constexpr std::string_view kHeaderLabelPrefix = "x-reduct-label-";
constexpr std::string_view kHeaderEntries = "x-reduct-entries";
constexpr std::string_view kHeaderStartTs = "x-reduct-start-ts";
constexpr std::string_view kHeaderLabels = "x-reduct-labels";

}  // namespace reduct::internal

#endif  // REDUCT_CPP_HEADERS_H
