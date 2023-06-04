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

  IBucket::Time ts = IBucket::Time() + std::chrono::microseconds(123109210);
  std::string blob = "some blob of data";
  REQUIRE(bucket->Write("entry",
                        IBucket::WriteOptions{
                            .timestamp = ts,
                            .labels = {{"label1", "value1"}, {"label2", "value2"}},
                            .content_type = "text/plain",
                        },
                        [&blob](auto rec) { rec->WriteAll(blob); }) == Error::kOk);

  std::string received_data;
  err = bucket->Read("entry", ts, [&received_data, ts](auto record) {
    REQUIRE(record.size == 17);
    REQUIRE(record.timestamp == ts);
    REQUIRE(record.labels == std::map<std::string, std::string>{{"label1", "value1"}, {"label2", "value2"}});
    REQUIRE(record.content_type == "text/plain");

    auto [data, read_err] = record.ReadAll();
    received_data = std::move(data);
    REQUIRE(read_err == Error::kOk);
    return true;
  });

  REQUIRE(err == Error::kOk);
  REQUIRE(received_data == blob);

  SECTION("http errors") {
    REQUIRE(bucket->Read("entry", IBucket::Time(), [](auto) { return true; }) ==
            Error{.code = 404, .message = "No record with timestamp 0"});
  }
}

TEST_CASE("reduct::IBucket should read the latest record", "[entry_api]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);
  REQUIRE(bucket);

  using us = std::chrono::microseconds;
  IBucket::Time ts{};
  REQUIRE(bucket->Write("entry", ts, [](auto rec) { rec->WriteAll("some_data1"); }) == Error::kOk);
  REQUIRE(bucket->Write("entry", ts + us(1), [](auto rec) { rec->WriteAll("some_data2"); }) == Error::kOk);
  REQUIRE(bucket->Write("entry", ts + us(2), [](auto rec) { rec->WriteAll("some_data3"); }) == Error::kOk);

  auto err = bucket->Read("entry", {}, [ts](auto record) {
    REQUIRE(record.size == 10);
    REQUIRE(record.timestamp == ts + us(2));

    return true;
  });

  REQUIRE(err == Error::kOk);
}

TEST_CASE("reduct::IBucket should read a record in chunks", "[entry_api]") {
  Fixture ctx;
  auto [bucket, err] = ctx.client->CreateBucket(kBucketName);

  REQUIRE(err == Error::kOk);
  REQUIRE(bucket);

  IBucket::Time ts = IBucket::Time::clock::now();
  const std::string blob(10'000, 'x');
  REQUIRE(bucket->Write("entry", ts, [&blob](auto rec) { rec->WriteAll(blob); }) == Error::kOk);

  std::string received;
  REQUIRE(bucket->Read("entry", ts, [&received](auto record) {
    auto record_err = record.Read([&received](auto data) {
      received.append(data);
      return true;
    });

    REQUIRE(record_err == Error::kOk);
    return true;
  }) == Error::kOk);

  REQUIRE(received == blob);
}

TEST_CASE("reduct::IBucket should write a record in chunks", "[entry_api]") {
  Fixture ctx;
  auto [bucket, err] = ctx.client->CreateBucket(kBucketName);

  REQUIRE(err == Error::kOk);
  REQUIRE(bucket);

  IBucket::Time ts = IBucket::Time::clock::now();
  const std::string blob(10'000, 'x');
  REQUIRE(bucket->Write("entry", ts, [&blob](auto rec) {
    rec->Write(blob.size(), [&](auto offset, auto size) {
      return std::pair{
          true,
          blob.substr(offset, size),
      };
    });
  }) == Error::kOk);

  err = bucket->Read("entry", ts, [&blob](auto record) {
    auto [data, read_err] = record.ReadAll();
    REQUIRE(read_err == Error::kOk);
    REQUIRE(data == blob);

    return true;
  });

  REQUIRE(err == Error::kOk);
}

TEST_CASE("reduct::IBucket should query records", "[entry_api]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);
  REQUIRE(bucket);

  using us = std::chrono::microseconds;
  IBucket::Time ts{};
  REQUIRE(bucket->Write("entry",
                        IBucket::WriteOptions{
                            .timestamp = ts,
                            .labels = {{"label1", "value1"}},
                        },
                        [](auto rec) { rec->WriteAll("some_data1"); }) == Error::kOk);
  REQUIRE(bucket->Write("entry", ts + us(1), [](auto rec) { rec->WriteAll("some_data2"); }) == Error::kOk);
  REQUIRE(bucket->Write("entry", ts + us(2), [](auto rec) { rec->WriteAll("some_data3"); }) == Error::kOk);

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

    REQUIRE(err == Error::kOk);
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

    REQUIRE(err == Error::kOk);
    REQUIRE(all_data == "some_data1");
  }

  SECTION("meta data about record") {
    auto err = bucket->Query("entry", ts, ts + us(1), {}, [ts](auto record) {
      REQUIRE(record.timestamp == ts);
      REQUIRE(record.size == 10);
      return true;
    });

    REQUIRE(err == Error::kOk);
  }

  SECTION("include labels") {
    auto err = bucket->Query("entry", ts, ts + us(3), IBucket::QueryOptions{.include = {{"label1", "value1"}}},
                             [&all_data](auto record) {
                               auto read_err = record.Read([&all_data](auto data) {
                                 all_data.append(data);
                                 return true;
                               });

                               REQUIRE(read_err == Error::kOk);
                               return true;
                             });

    REQUIRE(err == Error::kOk);
    REQUIRE(all_data == "some_data1");
  }

  SECTION("exclude labels") {
    auto err = bucket->Query("entry", ts, ts + us(3), IBucket::QueryOptions{.exclude = {{"label1", "value1"}}},
                             [&all_data](auto record) {
                               auto read_err = record.Read([&all_data](auto data) {
                                 all_data.append(data);
                                 return true;
                               });

                               REQUIRE(read_err == Error::kOk);
                               return true;
                             });

    REQUIRE(err == Error::kOk);
    REQUIRE(all_data == "some_data2some_data3");
  }
}

TEST_CASE("reduct::IBucket should query records (huge blob)", "[entry_api]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);
  REQUIRE(bucket);

  using us = std::chrono::microseconds;
  IBucket::Time ts{};
  std::string blob(100000, 'x');
  REQUIRE(bucket->Write("entry", ts, [&blob](auto rec) { rec->WriteAll(blob); }) == Error::kOk);

  std::string received_data;
  auto err = bucket->Query("entry", ts, ts + us(3), {}, [&received_data](auto record) {
    auto read_err = record.Read([&received_data](auto data) {
      received_data.append(data);
      return true;
    });

    REQUIRE(read_err == Error::kOk);
    return false;
  });

  REQUIRE(err == Error::kOk);
  REQUIRE(received_data == blob);
}
