// Copyright 2022 Alexey Timin

#include <catch2/catch.hpp>

#include "helpers.h"
#include "reduct/client.h"

using reduct::Error;
using reduct::IClient;

TEST_CASE("reduct::Client should create a bucket", "[bucket_api]") {
  StorageLauncher launcher;

  auto client = IClient::Build("http://127.0.0.1:8383");

  auto [bucket, err] = client->CreateBucket("bucket");

  REQUIRE(err == Error::kOk);
  REQUIRE(bucket);

  SECTION("should return HTTP status") {
    auto [null, err_409] = client->CreateBucket("bucket");
    REQUIRE(err_409 == Error{.code = 409, .message = "Bucket 'bucket' already exists"});
    REQUIRE_FALSE(null);
  }
}
