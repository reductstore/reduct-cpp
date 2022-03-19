// Copyright 2022 Alexey Timin

#include <catch2/catch.hpp>

#include "helpers.h"
#include "reduct/client.h"

using reduct::Error;
using reduct::IClient;

TEST_CASE("reduct::Client should get info", "[server_api]") {
  auto client = BuildClient();
  auto [info, err] = client->GetInfo();

  REQUIRE(err == Error::kOk);
  REQUIRE(info.version == "0.3.0");
  REQUIRE(info.bucket_count > 0);
  REQUIRE(info.usage >= 0);
  REQUIRE(info.uptime.count() >= 0);
  REQUIRE(info.oldest_record.time_since_epoch().count() >= 0);
  REQUIRE(info.latest_record.time_since_epoch().count() >= 0);
}

TEST_CASE("reduct::Client should list buckets", "[server_api]") {
  auto client = BuildClient();
  auto [list, err] = client->GetBucketList();

  REQUIRE_FALSE(list.empty());
  REQUIRE_FALSE(list[0].name.empty());
  REQUIRE(list[0].size >= 0);
  REQUIRE(list[0].entry_count >= 0);
  REQUIRE(list[0].oldest_record.time_since_epoch().count() >= 0);
  REQUIRE(list[0].latest_record.time_since_epoch().count() >= 0);
}

TEST_CASE("reduct::Client should return error", "[server_api]") {
  auto client = BuildClient("http://128.11.1.1:8383");
  auto [info, err] = client->GetInfo();

  REQUIRE(err == Error{.code = -1, .message = "Connection"});
}
