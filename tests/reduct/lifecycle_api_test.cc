// Copyright 2022-2026 ReductSoftware UG

#include <catch2/catch.hpp>

#include "fixture.h"
#include "reduct/client.h"

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
}

TEST_CASE("reduct::Client should create a lifecycle", "[lifecycle_api][1_20]") {
  Fixture ctx;
  auto settings = DefaultSettings();
  settings.type = IClient::LifecycleType::kCompress;

  auto err = ctx.client->CreateLifecycle("test_lifecycle", settings);
  REQUIRE(err == Error::kOk);

  auto [lifecycle, err_2] = ctx.client->GetLifecycle("test_lifecycle");
  REQUIRE(err_2 == Error::kOk);
  REQUIRE(lifecycle.info == IClient::LifecycleInfo{
                                .name = "test_lifecycle",
                                .mode = IClient::LifecycleMode::kEnabled,
                                .is_provisioned = false,
                                .is_running = true,
                            });

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