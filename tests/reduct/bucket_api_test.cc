// Copyright 2022 Alexey Timin

#include <catch2/catch.hpp>

#include "helpers.h"
#include "reduct/client.h"

using reduct::Error;
using reduct::IClient;

TEST_CASE("reduct::Client should create a bucket", "[bucket_api]") {
  auto client = IClient::Build("http://127.0.0.1:8383");

  const auto kBucketName = RandomBucketName();
  auto [bucket, err] = client->CreateBucket(kBucketName);

  REQUIRE(err == Error::kOk);
  REQUIRE(bucket);

  SECTION("should return HTTP status") {
    auto [null, err_409] = client->CreateBucket(kBucketName);
    REQUIRE(err_409 == Error{.code = 409, .message = fmt::format("Bucket '{}' already exists", kBucketName)});
    REQUIRE_FALSE(null);
  }
}
