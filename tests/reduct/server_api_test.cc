// Copyright 2022 Alexey Timin

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
  REQUIRE(info.version >= "1.2.0");

  REQUIRE(info.bucket_count == 2);
  REQUIRE(info.usage == 234);
  REQUIRE(info.uptime.count() >= 1);
  REQUIRE(info.oldest_record.time_since_epoch() == s(1));
  REQUIRE(info.latest_record.time_since_epoch() == s(6));

  REQUIRE(*info.defaults.bucket.max_block_size == 64000000);
  // REQUIRE(*info.defaults.bucket.max_block_records == 1024); TODO: get back in 1.7
  REQUIRE(*info.defaults.bucket.quota_type == IBucket::QuotaType::kNone);
  REQUIRE(*info.defaults.bucket.quota_size == 0);
}

TEST_CASE("reduct::Client should list buckets", "[server_api]") {
  Fixture ctx;
  auto [list, err] = ctx.client->GetBucketList();

  REQUIRE_FALSE(list.empty());
  REQUIRE(list[0].name == "bucket_1");
  REQUIRE(list[0].size == 156);
  REQUIRE(list[0].entry_count == 2);
  REQUIRE(list[0].oldest_record.time_since_epoch() == s(1));
  REQUIRE(list[0].latest_record.time_since_epoch() == s(4));

  REQUIRE(list[1].name == "bucket_2");
  REQUIRE(list[1].size == 78);
  REQUIRE(list[1].entry_count == 1);
  REQUIRE(list[1].oldest_record.time_since_epoch() == s(5));
  REQUIRE(list[1].latest_record.time_since_epoch() == s(6));
}

TEST_CASE("reduct::Client should return error", "[server_api]") {
  auto client = IClient::Build("http://127.0.0.1:99999");
  auto [info, err] = client->GetInfo();

  REQUIRE(err == Error{.code = -1, .message = "Could not establish connection"});
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
