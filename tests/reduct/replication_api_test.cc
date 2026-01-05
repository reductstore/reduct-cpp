// Copyright 2022-2024 ReductSoftware UG

#include <catch2/catch.hpp>

#include <thread>

#include "fixture.h"
#include "reduct/client.h"

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

TEST_CASE("reduct::Client should set replication mode", "[replication_api][1_18]") {
  Fixture ctx;
  auto settings = DefaultSettings();
  auto err = ctx.client->CreateReplication("test_replication", settings);
  REQUIRE(err == Error::kOk);

  err = ctx.client->SetReplicationMode("test_replication", IClient::ReplicationMode::kPaused);
  REQUIRE(err == Error::kOk);

  auto [replication, err_2] = ctx.client->GetReplication("test_replication");
  REQUIRE(err_2 == Error::kOk);
  REQUIRE(replication.info == IClient::ReplicationInfo{
                                  .name = "test_replication",
                                  .mode = IClient::ReplicationMode::kPaused,
                                  .is_active = false,
                                  .is_provisioned = false,
                                  .pending_records = 0,
                              });
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
  settings.each_s = 1.5;
  settings.each_n = 10;

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
