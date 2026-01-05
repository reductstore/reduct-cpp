// Copyright 2022-2025 ReductSoftware UG

#include <catch2/catch.hpp>

#include <thread>

#include "fixture.h"
#include "reduct/client.h"

using reduct::Error;
using reduct::IBucket;
using reduct::IClient;
using s = std::chrono::seconds;

TEST_CASE("reduct::Client should get info", "[server_api]") {
  Fixture ctx;
  std::this_thread::sleep_for(s(1));

  auto [info, err] = ctx.client->GetInfo();

  REQUIRE(err == Error::kOk);
  REQUIRE(info.version >= "1.10.0");

  REQUIRE(info.bucket_count == 2);
  REQUIRE(info.usage == 234);
  REQUIRE(info.uptime.count() >= 1);
  REQUIRE(info.oldest_record.time_since_epoch() == s(1));
  REQUIRE(info.latest_record.time_since_epoch() == s(6));

  REQUIRE(*info.defaults.bucket.max_block_size == 64000000);
  REQUIRE(*info.defaults.bucket.max_block_records == 1024);
  REQUIRE(*info.defaults.bucket.quota_type == IBucket::QuotaType::kNone);
  REQUIRE(*info.defaults.bucket.quota_size == 0);
}

TEST_CASE("reduct::Client should get license info", "[server_api][license]") {
  Fixture ctx;
  auto [info, err] = ctx.client->GetInfo();

  REQUIRE(err == Error::kOk);
  REQUIRE(info.license);
  REQUIRE(info.license->licensee == "ReductSoftware");
  REQUIRE(info.license->invoice == "---");
  REQUIRE(info.license->expiry_date.time_since_epoch().count() == 1778852143000000000);
  REQUIRE(info.license->plan == "STANDARD");
  REQUIRE(info.license->device_number == 1);
  REQUIRE(info.license->disk_quota == 1);
  REQUIRE(info.license->fingerprint == "21e2608b7d47f7fba623d714c3e14b73cd1fe3578f4010ef26bcbedfc42a4c92");
}

TEST_CASE("reduct::Client should list buckets", "[server_api]") {
  Fixture ctx;
  auto [list, err] = ctx.client->GetBucketList();

  REQUIRE_FALSE(list.empty());
  REQUIRE(list[0].name == "test_bucket_1");
  REQUIRE(list[0].size == 156);
  REQUIRE(list[0].entry_count == 2);
  REQUIRE(list[0].oldest_record.time_since_epoch() == s(1));
  REQUIRE(list[0].latest_record.time_since_epoch() == s(4));
  REQUIRE(list[0].status == IBucket::Status::kReady);

  REQUIRE(list[1].name == "test_bucket_2");
  REQUIRE(list[1].size == 78);
  REQUIRE(list[1].entry_count == 1);
  REQUIRE(list[1].oldest_record.time_since_epoch() == s(5));
  REQUIRE(list[1].latest_record.time_since_epoch() == s(6));
  REQUIRE(list[1].status == IBucket::Status::kReady);
}

TEST_CASE("reduct::Client should return error", "[server_api]") {
  auto client = IClient::Build("http://127.0.0.1:99999");
  auto [info, err] = client->GetInfo();

  REQUIRE(err == Error{.code = -1, .message = "Could not establish connection"});
}

TEST_CASE("reduct::Client should return 404 if base path wrong") {
  auto client = IClient::Build("http://127.0.0.1:8383/wrong_path");
  auto [info, err] = client->GetInfo();
  REQUIRE(err == Error{.code = 404, .message = "Not found"});
}

TEST_CASE("reduct::Client should return current token name and permissions", "[server_api][token_api]") {
  Fixture ctx;
  auto [token, err] = ctx.client->Me();

  REQUIRE(err == Error::kOk);
  REQUIRE(token.name == "init-token");
  REQUIRE(token.permissions.full_access == true);
  REQUIRE(token.permissions.read.empty());
  REQUIRE(token.permissions.write.empty());
}
