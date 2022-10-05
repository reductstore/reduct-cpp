// Copyright 2022 Alexey Timin

#include <catch2/catch.hpp>

#include <fstream>
#include <sstream>

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

TEST_CASE("reduct::IBucket should write a record by chunks", "[entry_api]") {
  Fixture ctx;
  auto [bucket, err] = ctx.client->CreateBucket(kBucketName);

  REQUIRE(err == Error::kOk);
  REQUIRE(bucket);

  IBucket::Time ts = IBucket::Time::clock::now();
  const std::string blob(10'000, 'x');
  REQUIRE(bucket->Write("entry", ts, blob.size(), [&blob](auto offset, auto size) {
    return std::pair{true, blob.substr(offset, size)};
  }) == Error::kOk);

  auto [received, read_err] = bucket->Read("entry", ts);
  REQUIRE(read_err == Error::kOk);
  REQUIRE(received == blob);
}

TEST_CASE("reduct::IBucket should query records", "[entry_api]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);
  REQUIRE(bucket);

  using us = std::chrono::microseconds;
  IBucket::Time ts{};
  REQUIRE(bucket->Write("entry", "some_data1", ts) == Error::kOk);
  REQUIRE(bucket->Write("entry", "some_data2", ts + us(1)) == Error::kOk);
  REQUIRE(bucket->Write("entry", "some_data3", ts + us(2)) == Error::kOk);

  std::string all_data;
  SECTION("receive all data") {
    auto err = bucket->Query("entry", ts, ts + us(3), {}, [&all_data](auto record) {
      auto read_err = record.Read([&all_data](auto data) {
        all_data.append(data);
        return true;
      });

      REQUIRE(read_err == Error::kOk);
      return true;
    });

    REQUIRE(all_data == "some_data1some_data2some_data3");
  }

  SECTION("stop receiving") {
    auto err = bucket->Query("entry", ts, ts + us(3), {}, [&all_data](auto record) {
      auto read_err = record.Read([&all_data](auto data) {
        all_data.append(data);
        return true;
      });

      REQUIRE(read_err == Error::kOk);
      return false;
    });

    REQUIRE(all_data == "some_data1");
  }

  SECTION("meta data about record") {
    auto err = bucket->Query("entry", ts, ts + us(1), {}, [ts](auto record) {
      REQUIRE(record.timestamp == ts);
      REQUIRE(record.size == 10);
      REQUIRE(record.last);
      return true;
    });
  }
}

TEST_CASE("reduct::IBucket should query records (huge blob)", "[entry_api]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);
  REQUIRE(bucket);

  using us = std::chrono::microseconds;
  IBucket::Time ts{};
  std::string blob(100000, 'x');
  REQUIRE(bucket->Write("entry", blob, ts) == Error::kOk);

  std::string received_data;
  auto err = bucket->Query("entry", ts, ts + us(3), {}, [&received_data](auto record) {
    auto read_err = record.Read([&received_data](auto data) {
      received_data.append(data);
      return true;
    });

    REQUIRE(read_err == Error::kOk);
    return false;
  });

  REQUIRE(received_data == blob);
}
