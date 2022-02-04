// Copyright 2022 Alexey Timin

#include <catch2/catch.hpp>

#include "helpers.h"
#include "reduct/client.h"

using reduct::Error;
using reduct::IBucket;
using reduct::IClient;

TEST_CASE("reduct::IBucket should write/read a record", "[entry_api]") {
  auto client = IClient::Build("http://127.0.0.1:8383");

  const auto kBucketName = RandomBucketName();
  auto [bucket, err] = client->CreateBucket(kBucketName);

  REQUIRE(err == Error::kOk);
  REQUIRE(bucket);

  IBucket::Time ts = IBucket::Time::clock::now();
  std::string_view blob("some blob of data");
  REQUIRE(bucket->Write("entry", blob, ts) == Error::kOk);
  REQUIRE(bucket->Read("entry", ts).result == blob);

  SECTION("http errors") {
    REQUIRE(bucket->Read("entry", IBucket::Time()) == Error{.code = 404, .message = "No records for this timestamp"});
  }
}

TEST_CASE("reduct::IBucket should list records", "[entry_api]") {
  auto client = IClient::Build("http://127.0.0.1:8383");

  const auto kBucketName = RandomBucketName();
  auto [bucket, _] = client->CreateBucket(kBucketName);
  REQUIRE(bucket);

  using us = std::chrono::microseconds;
  IBucket::Time ts{};
  REQUIRE(bucket->Write("entry", "some_data1", ts) == Error::kOk);
  REQUIRE(bucket->Write("entry", "some_data1", ts + us(1)) == Error::kOk);
  REQUIRE(bucket->Write("entry", "some_data1", ts + us(2)) == Error::kOk);

  auto [list, err] = bucket->List("entry", ts, ts + us(3));

  REQUIRE(err == Error::kOk);
  REQUIRE(list.size() == 3);
  REQUIRE(list[0] == IBucket::RecordInfo{.timestamp = ts, .size = 12});
  REQUIRE(list[1] == IBucket::RecordInfo{.timestamp = ts + us(1), .size = 12});
  REQUIRE(list[2] == IBucket::RecordInfo{.timestamp = ts + us(2), .size = 12});

  SECTION("http error") {
    REQUIRE(bucket->List("entry", ts + us(3), ts + us(10)).error ==
            Error{.code = 404, .message = "No records for time interval"});
  }
}
