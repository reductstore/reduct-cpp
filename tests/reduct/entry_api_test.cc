// Copyright 2022 Alexey Timin

#include <catch2/catch.hpp>

#include "fixture.h"
#include "reduct/client.h"

using reduct::Error;
using reduct::IBucket;
using reduct::IClient;

const auto kBucketName = "bucket";

TEST_CASE("reduct::IBucket should write/read a record", "[entry_api]") {
  Fixture ctx;
  auto [bucket, err] = ctx.client->CreateBucket(kBucketName);

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

TEST_CASE("reduct::IBucket should read the latest record", "[entry_api]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);
  REQUIRE(bucket);

  using us = std::chrono::microseconds;
  IBucket::Time ts{};
  REQUIRE(bucket->Write("entry", "some_data1", ts) == Error::kOk);
  REQUIRE(bucket->Write("entry", "some_data2", ts + us(1)) == Error::kOk);
  REQUIRE(bucket->Write("entry", "some_data3", ts + us(2)) == Error::kOk);

  auto [latest_record, err] = bucket->Read("entry");
  REQUIRE(err == Error::kOk);
  REQUIRE(latest_record == "some_data3");
}

TEST_CASE("reduct::IBucket should read a record by chunks", "[entry_api]") {
  Fixture ctx;
  auto [bucket, err] = ctx.client->CreateBucket(kBucketName);

  REQUIRE(err == Error::kOk);
  REQUIRE(bucket);

  IBucket::Time ts = IBucket::Time::clock::now();
  const std::string blob(10'000, 'x');
  REQUIRE(bucket->Write("entry", blob, ts) == Error::kOk);

  std::string received;
  REQUIRE(bucket->Read("entry", ts, [&received](auto data) {
    received.append(data);
    return true;
  }) == Error::kOk);

  REQUIRE(received == blob);
}

TEST_CASE("reduct::IBucket should list records", "[entry_api]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);
  REQUIRE(bucket);

  using us = std::chrono::microseconds;
  IBucket::Time ts{};
  REQUIRE(bucket->Write("entry", "some_data1", ts) == Error::kOk);
  REQUIRE(bucket->Write("entry", "some_data1", ts + us(1)) == Error::kOk);
  REQUIRE(bucket->Write("entry", "some_data1", ts + us(2)) == Error::kOk);

  auto [list, err] = bucket->List("entry", ts, ts + us(3));

  REQUIRE(err == Error::kOk);
  REQUIRE(list.size() == 3);
  REQUIRE(list[0] == IBucket::RecordInfo{.timestamp = ts, .size = 10});
  REQUIRE(list[1] == IBucket::RecordInfo{.timestamp = ts + us(1), .size = 10});
  REQUIRE(list[2] == IBucket::RecordInfo{.timestamp = ts + us(2), .size = 10});

  SECTION("http error") {
    REQUIRE(bucket->List("entry", ts + us(3), ts + us(10)).error ==
            Error{.code = 404, .message = "No records for time interval"});
  }
}
