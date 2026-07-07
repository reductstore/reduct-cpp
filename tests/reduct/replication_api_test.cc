// Copyright 2022-2024 ReductSoftware UG

#include <catch2/catch.hpp>

#include <thread>

#include "fixture.h"
#include "reduct/client.h"
#include "reduct/internal/serialisation.h"

using reduct::Error;
using reduct::IClient;

namespace {

IClient::ReplicationSettings DefaultSettings() {
  return {
      .src_bucket = "test_bucket_1",
      .dst_bucket = "test_bucket_2",
      .dst_host = "http://127.0.0.1:8383",
      .entries = {"entry-1"},
      .mode = IClient::ReplicationMode::kEnabled,
  };
}

}  // namespace

TEST_CASE("reduct::Client should get list of replications", "[replication_api][1_8]") {
  Fixture ctx;
  auto [replications, err] = ctx.client->GetReplicationList();
  REQUIRE(err == Error::kOk);
  REQUIRE(replications.size() == 0);
}

TEST_CASE("reduct::Client should create a replication", "[replication_api][1_17]") {
  Fixture ctx;
  auto settings = DefaultSettings();

  auto err = ctx.client->CreateReplication("test_replication", settings);
  REQUIRE(err == Error::kOk);

  auto [replication, err_2] = ctx.client->GetReplication("test_replication");
  REQUIRE(err_2 == Error::kOk);
  REQUIRE(replication.info == IClient::ReplicationInfo{
                                  .name = "test_replication",
                                  .mode = IClient::ReplicationMode::kEnabled,
                                  .is_active = true,
                                  .is_provisioned = false,
                                  .pending_records = 0,
                              });

  REQUIRE(replication.settings == settings);
  REQUIRE(replication.diagnostics == reduct::Diagnostics{});

  SECTION("Conflict") {
    REQUIRE(ctx.client->CreateReplication("test_replication", {}) ==
            Error{409, "Replication 'test_replication' already exists"});
  }
}

TEST_CASE("reduct::Client should update a replication", "[replication_api][1_17]") {
  Fixture ctx;
  auto settings = DefaultSettings();
  auto err = ctx.client->CreateReplication("test_replication", settings);
  REQUIRE(err == Error::kOk);

  settings.entries = {"entry-2"};
  err = ctx.client->UpdateReplication("test_replication", settings);
  REQUIRE(err == Error::kOk);

  auto [replication, err_2] = ctx.client->GetReplication("test_replication");
  REQUIRE(err_2 == Error::kOk);

  REQUIRE(replication.settings == settings);

  SECTION("Not found") {
    REQUIRE(ctx.client->UpdateReplication("test_replication_2", {}) ==
            Error{404, "Replication 'test_replication_2' does not exist"});
  }
}

TEST_CASE("reduct::Client should serialize replication destination prefix", "[replication_api]") {
  auto settings = DefaultSettings();

  SECTION("omits empty destination prefix") {
    auto [json, err] = reduct::internal::ReplicationSettingsToJsonString(settings);

    REQUIRE(err == Error::kOk);
    REQUIRE_FALSE(json.contains("dst_prefix"));
  }

  SECTION("serializes non-empty destination prefix") {
    settings.dst_prefix = "robot-1";

    auto [json, err] = reduct::internal::ReplicationSettingsToJsonString(settings);

    REQUIRE(err == Error::kOk);
    REQUIRE(json.at("dst_prefix") == "robot-1");
  }
}

TEST_CASE("reduct::Client should parse replication destination prefix", "[replication_api]") {
  auto response = nlohmann::json{
      {"info",
       {{"name", "test_replication"},
        {"mode", "enabled"},
        {"is_active", true},
        {"is_provisioned", false},
        {"pending_records", 0}}},
      {"settings",
       {{"src_bucket", "test_bucket_1"},
        {"dst_bucket", "test_bucket_2"},
        {"dst_host", "http://127.0.0.1:8383"},
        {"entries", {"entry-1"}},
        {"mode", "enabled"}}},
      {"diagnostics", {{"hourly", {{"ok", 0}, {"errored", 0}, {"errors", nlohmann::json::object()}}}}},
  };

  SECTION("keeps destination prefix empty when absent") {
    auto [replication, err] = reduct::internal::ParseFullReplicationInfo(response);

    REQUIRE(err == Error::kOk);
    REQUIRE(replication.settings.dst_prefix.empty());
  }

  SECTION("parses returned destination prefix") {
    response["settings"]["dst_prefix"] = "robot-1";

    auto [replication, err] = reduct::internal::ParseFullReplicationInfo(response);

    REQUIRE(err == Error::kOk);
    REQUIRE(replication.settings.dst_prefix == "robot-1");
  }
}

TEST_CASE("reduct::Client should set replication destination prefix", "[replication_api][1_20]") {
  Fixture ctx;
  auto settings = DefaultSettings();
  settings.dst_prefix = "robot-1";

  auto err = ctx.client->CreateReplication("test_replication", settings);
  REQUIRE(err == Error::kOk);

  auto [replication, err_2] = ctx.client->GetReplication("test_replication");
  REQUIRE(err_2 == Error::kOk);
  REQUIRE(replication.settings.dst_prefix == "robot-1");

  settings.dst_prefix = "line-a";
  err = ctx.client->UpdateReplication("test_replication", settings);
  REQUIRE(err == Error::kOk);

  auto [updated_replication, err_3] = ctx.client->GetReplication("test_replication");
  REQUIRE(err_3 == Error::kOk);
  REQUIRE(updated_replication.settings.dst_prefix == "line-a");
}

TEST_CASE("reduct::Client should set replication mode", "[replication_api][1_18]") {
  Fixture ctx;
  auto settings = DefaultSettings();
  auto err = ctx.client->CreateReplication("test_replication", settings);
  REQUIRE(err == Error::kOk);

  err = ctx.client->SetReplicationMode("test_replication", IClient::ReplicationMode::kPaused);
  REQUIRE(err == Error::kOk);

  auto [replication, err_2] = ctx.client->GetReplication("test_replication");
  REQUIRE(err_2 == Error::kOk);
  REQUIRE(replication.settings.mode == IClient::ReplicationMode::kPaused);
  REQUIRE(replication.settings.entries == settings.entries);

  SECTION("Not found") {
    REQUIRE(ctx.client->SetReplicationMode("unknown", IClient::ReplicationMode::kDisabled) ==
            Error{404, "Replication 'unknown' does not exist"});
  }
}

TEST_CASE("reduct::Client should remove a replication", "[replication_api][1_8]") {
  Fixture ctx;
  auto settings = DefaultSettings();
  auto err = ctx.client->CreateReplication("test_replication", settings);
  REQUIRE(err == Error::kOk);

  err = ctx.client->RemoveReplication("test_replication");
  REQUIRE(err == Error::kOk);

  auto [replication, err_2] = ctx.client->GetReplication("test_replication");
  REQUIRE(err_2 == Error{404, "Replication 'test_replication' does not exist"});

  SECTION("Not found") {
    REQUIRE(ctx.client->RemoveReplication("test_replication_2") ==
            Error{404, "Replication 'test_replication_2' does not exist"});
  }
}

TEST_CASE("reduct::Client should set each_s and each_n settings", "[replication_api][1_17]") {
  Fixture ctx;
  auto settings = DefaultSettings();
  auto err = ctx.client->CreateReplication("test_replication", settings);
  REQUIRE(err == Error::kOk);

  auto [replication, err_2] = ctx.client->GetReplication("test_replication");
  REQUIRE(err_2 == Error::kOk);
  REQUIRE(replication.info == IClient::ReplicationInfo{
                                  .name = "test_replication",
                                  .mode = IClient::ReplicationMode::kEnabled,
                                  .is_active = true,
                                  .is_provisioned = false,
                                  .pending_records = 0,
                              });

  REQUIRE(replication.settings == settings);
}

TEST_CASE("reduct::Client should set when condition", "[replication_api][1_14]") {
  Fixture ctx;
  auto settings = DefaultSettings();
  settings.when = R"({"&score":{"$gt":0}})";

  auto err = ctx.client->CreateReplication("test_replication", settings);
  REQUIRE(err == Error::kOk);

  auto [replication, err_2] = ctx.client->GetReplication("test_replication");
  REQUIRE(err_2 == Error::kOk);
  REQUIRE(replication.settings.when == settings.when);
}
