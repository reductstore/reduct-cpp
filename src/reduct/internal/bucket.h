// Copyright 2026 ReductSoftware UG
#ifndef REDUCT_CPP_INTERNAL_BUCKET_H
#define REDUCT_CPP_INTERNAL_BUCKET_H

#include <memory>
#include <string_view>

#include "reduct/bucket.h"
#include "reduct/internal/http_client.h"

namespace reduct::internal {

std::unique_ptr<IBucket> BuildBucketForTesting(std::string_view name, std::unique_ptr<IHttpClient> client) noexcept;

}  // namespace reduct::internal

#endif  // REDUCT_CPP_INTERNAL_BUCKET_H
