// Copyright 2024 Alexey Timin

#include <catch2/catch.hpp>

#include <thread>

#include "fixture.h"
#include "reduct/client.h"

using reduct::Error;
using reduct::IClient;

TEST_CASE("reduct::Client should get list of replications", "[replication_api]") {
  Fixture ctx;
  auto [tokens, err] = ctx.client->GetTokenList();
  REQUIRE(err == Error::kOk);
  REQUIRE(tokens.size() == 1);
  REQUIRE(tokens[0].name == "init-token");
  REQUIRE(tokens[0].created_at.time_since_epoch().count() > 0);
}
