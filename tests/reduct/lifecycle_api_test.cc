// Copyright 2022-2026 ReductSoftware UG

#include <catch2/catch.hpp>

#include <algorithm>
#include <chrono>

#include <nlohmann/json.hpp>

#include "fixture.h"
#include "reduct/client.h"
#include "reduct/internal/serialisation.h"

using reduct::Error;
using reduct::IClient;

namespace {

IClient::LifecycleSettings DefaultSettings() {
  return {
      .type = IClient::LifecycleType::kDelete,
      .bucket = "test_bucket_1",
      .entries = {"entry-1"},
      .older_than = "1h",
      .interval = "10m",
      .mode = IClient::LifecycleMode::kEnabled,
  };
}

}  // namespace

TEST_CASE("reduct::Client should get list of lifecycles", "[lifecycle_api][1_20]") {
  Fixture ctx;
  auto [lifecycles, err] = ctx.client->GetLifecycleList();
  REQUIRE(err == Error::kOk);
  for (const auto& lifecycle : lifecycles) {
    REQUIRE_FALSE(lifecycle.name.starts_with("test_lifecycle"));
  }

  auto settings = DefaultSettings();
  settings.type = IClient::LifecycleType::kCompress;
  REQUIRE(ctx.client->CreateLifecycle("test_lifecycle", settings) == Error::kOk);

  auto [updated_lifecycles, err_2] = ctx.client->GetLifecycleList();
  REQUIRE(err_2 == Error::kOk);

  auto it = std::find_if(updated_lifecycles.begin(), updated_lifecycles.end(), [](const auto& lifecycle) {
    return lifecycle.name == "test_lifecycle";
  });
  REQUIRE(it != updated_lifecycles.end());
  REQUIRE(it->type == IClient::LifecycleType::kCompress);
  REQUIRE_FALSE(it->last_run.has_value());
}

TEST_CASE("reduct::Client should create a lifecycle", "[lifecycle_api][1_20]") {
  Fixture ctx;
  auto settings = DefaultSettings();
  settings.type = IClient::LifecycleType::kCompress;

  auto err = ctx.client->CreateLifecycle("test_lifecycle", settings);
  REQUIRE(err == Error::kOk);

  auto [lifecycle, err_2] = ctx.client->GetLifecycle("test_lifecycle");
  REQUIRE(err_2 == Error::kOk);
  REQUIRE(lifecycle.info.name == "test_lifecycle");
  REQUIRE(lifecycle.info.type == IClient::LifecycleType::kCompress);
  REQUIRE(lifecycle.info.mode == IClient::LifecycleMode::kEnabled);
  REQUIRE_FALSE(lifecycle.info.is_provisioned);
  REQUIRE(lifecycle.info.is_running);
  REQUIRE_FALSE(lifecycle.info.last_run.has_value());
  REQUIRE(lifecycle.settings == settings);

  SECTION("Conflict") {
    REQUIRE(ctx.client->CreateLifecycle("test_lifecycle", settings) ==
            Error{409, "Lifecycle 'test_lifecycle' already exists"});
  }
}

TEST_CASE("reduct::Client should update a lifecycle", "[lifecycle_api][1_20]") {
  Fixture ctx;
  auto settings = DefaultSettings();
  auto err = ctx.client->CreateLifecycle("test_lifecycle", settings);
  REQUIRE(err == Error::kOk);

  settings.entries = {"entry-2"};
  settings.older_than = "2h";
  settings.interval = "20m";
  err = ctx.client->UpdateLifecycle("test_lifecycle", settings);
  REQUIRE(err == Error::kOk);

  auto [lifecycle, err_2] = ctx.client->GetLifecycle("test_lifecycle");
  REQUIRE(err_2 == Error::kOk);
  REQUIRE(lifecycle.settings == settings);

  SECTION("Not found") {
    REQUIRE(ctx.client->UpdateLifecycle("test_lifecycle_2", settings) ==
            Error{404, "Lifecycle 'test_lifecycle_2' does not exist"});
  }
}

TEST_CASE("reduct::Client should set lifecycle mode", "[lifecycle_api][1_20]") {
  Fixture ctx;
  auto settings = DefaultSettings();
  auto err = ctx.client->CreateLifecycle("test_lifecycle", settings);
  REQUIRE(err == Error::kOk);

  err = ctx.client->SetLifecycleMode("test_lifecycle", IClient::LifecycleMode::kDryRun);
  REQUIRE(err == Error::kOk);

  auto [lifecycle, err_2] = ctx.client->GetLifecycle("test_lifecycle");
  REQUIRE(err_2 == Error::kOk);
  REQUIRE(lifecycle.settings.mode == IClient::LifecycleMode::kDryRun);
  REQUIRE(lifecycle.settings.entries == settings.entries);

  SECTION("Not found") {
    REQUIRE(ctx.client->SetLifecycleMode("unknown", IClient::LifecycleMode::kDisabled) ==
            Error{404, "Lifecycle 'unknown' does not exist"});
  }
}

TEST_CASE("reduct::Client should remove a lifecycle", "[lifecycle_api][1_20]") {
  Fixture ctx;
  auto settings = DefaultSettings();
  auto err = ctx.client->CreateLifecycle("test_lifecycle", settings);
  REQUIRE(err == Error::kOk);

  err = ctx.client->RemoveLifecycle("test_lifecycle");
  REQUIRE(err == Error::kOk);

  auto [lifecycle, err_2] = ctx.client->GetLifecycle("test_lifecycle");
  REQUIRE(err_2 == Error{404, "Lifecycle 'test_lifecycle' does not exist"});

  SECTION("Not found") {
    REQUIRE(ctx.client->RemoveLifecycle("test_lifecycle_2") ==
            Error{404, "Lifecycle 'test_lifecycle_2' does not exist"});
  }
}

TEST_CASE("reduct::Client should set lifecycle when condition", "[lifecycle_api][1_20]") {
  Fixture ctx;
  auto settings = DefaultSettings();
  settings.when = R"({"&score":{"$gt":0}})";

  auto err = ctx.client->CreateLifecycle("test_lifecycle", settings);
  REQUIRE(err == Error::kOk);

  auto [lifecycle, err_2] = ctx.client->GetLifecycle("test_lifecycle");
  REQUIRE(err_2 == Error::kOk);
  REQUIRE(lifecycle.settings.when == settings.when);
}

TEST_CASE("reduct::Client should parse lifecycle type and RFC3339 last_run", "[lifecycle_api][unit]") {
  auto lifecycle_list_json = nlohmann::json::parse(R"({
    "lifecycles": [
      {
        "name": "test_lifecycle",
        "type": "compress",
        "mode": "enabled",
        "is_provisioned": false,
        "is_running": true,
        "last_run": "2026-06-15T06:44:40.123456Z"
      }
    ]
  })");

  auto [lifecycles, list_err] = reduct::internal::ParseLifecycleList(lifecycle_list_json);
  REQUIRE(list_err == Error::kOk);
  REQUIRE(lifecycles.size() == 1);
  REQUIRE(lifecycles[0].type == IClient::LifecycleType::kCompress);
  REQUIRE(lifecycles[0].last_run.has_value());
  REQUIRE(std::chrono::duration_cast<std::chrono::microseconds>(lifecycles[0].last_run->time_since_epoch()).count() %
              1000000 == 123456);

  auto full_lifecycle_json = nlohmann::json::parse(R"({
    "info": {
      "name": "test_lifecycle",
      "type": "compress",
      "mode": "enabled",
      "is_provisioned": false,
      "is_running": true,
      "last_run": "2026-06-15T06:44:40.654321Z"
    },
    "settings": {
      "type": "compress",
      "bucket": "test_bucket_1",
      "entries": ["entry-1"],
      "older_than": "1h",
      "mode": "enabled"
    }
  })");

  auto [full_lifecycle, full_err] = reduct::internal::ParseFullLifecycleInfo(full_lifecycle_json);
  REQUIRE(full_err == Error::kOk);
  REQUIRE(full_lifecycle.info.type == IClient::LifecycleType::kCompress);
  REQUIRE(full_lifecycle.info.last_run.has_value());
  REQUIRE(std::chrono::duration_cast<std::chrono::microseconds>(
              full_lifecycle.info.last_run->time_since_epoch())
              .count() %
              1000000 == 654321);
}
