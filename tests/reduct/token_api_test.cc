// Copyright 2022 Alexey Timin

#include <catch2/catch.hpp>

#include <thread>

#include "fixture.h"
#include "reduct/client.h"

using reduct::Error;
using reduct::IClient;

TEST_CASE("reduct::Client should get token list", "[token_api]") {
  Fixture ctx;
  auto [tokens, err] = ctx.client->GetTokenList();
  REQUIRE(err == Error::kOk);
  REQUIRE(tokens.size() == 1);
  REQUIRE(tokens[0].name == "init-token");
  REQUIRE(tokens[0].created_at.time_since_epoch().count() > 0);
}

TEST_CASE("reduct::Client should create token", "[token_api]") {
  Fixture ctx;
  IClient::Permissions permissions{
      .full_access = true,
      .read = {"bucket_1"},
      .write = {"bucket_2"},
  };

  auto [token, err] = ctx.client->CreateToken("test-token", std::move(permissions));
  REQUIRE(err == Error::kOk);
  REQUIRE(token.starts_with("test-token-"));

  SECTION("Conflict") {
    REQUIRE(ctx.client->CreateToken("test-token", {}).error == Error{409, "Token 'test-token' already exists"});
  }
}

TEST_CASE("reduct::Client should get token", "[token_api]") {
  Fixture ctx;
  IClient::Permissions permissions{
      .full_access = true,
      .read = {"bucket_1"},
      .write = {"bucket_2"},
  };

  {
    auto [_, err] = ctx.client->CreateToken("test-token", permissions);
    REQUIRE(err == Error::kOk);
  }

  auto [token, err] = ctx.client->GetToken("test-token");
  REQUIRE(err == Error::kOk);
  REQUIRE(token.name == "test-token");
  REQUIRE(token.created_at.time_since_epoch().count() > 0);
  REQUIRE(token.permissions.full_access == permissions.full_access);
  REQUIRE(token.permissions.read == permissions.read);
  REQUIRE(token.permissions.write == permissions.write);

  SECTION("not found") {
    REQUIRE(ctx.client->GetToken("not-found").error == Error{404, "Token 'not-found' doesn't exist"});
  }
}

TEST_CASE("reduct::Client should delete token", "[token_api]") {
  Fixture ctx;
  {
    auto [_, err] = ctx.client->CreateToken("test-token", {});
    REQUIRE(err == Error::kOk);
  }

  auto err = ctx.client->RemoveToken("test-token");
  REQUIRE(err == Error::kOk);

  SECTION("not found") {
    REQUIRE(ctx.client->RemoveToken("not-found") == Error{404, "Token 'not-found' doesn't exist"});
  }
}