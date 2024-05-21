// Copyright 2022-2024 Alexey Timin

#include <catch2/catch.hpp>

#include <thread>

#include "fixture.h"
#include "reduct/client.h"

using reduct::Error;
using reduct::IClient;

const std::string_view kTestTokenName = "test_token";

TEST_CASE("reduct::Client should get token list", "[token_api]") {
  Fixture ctx;
  auto [tokens, err] = ctx.client->GetTokenList();
  REQUIRE(err == Error::kOk);
  REQUIRE(tokens.size() == 1);
  REQUIRE(tokens[0].name == "init-token");
  REQUIRE(tokens[0].created_at.time_since_epoch().count() > 0);
  REQUIRE_FALSE(tokens[0].is_provisioned);
}

TEST_CASE("reduct::Client should create token", "[token_api]") {
  Fixture ctx;
  IClient::Permissions permissions{
      .full_access = true,
      .read = {"test_bucket_1"},
      .write = {"test_bucket_2"},
  };

  auto [token, err] = ctx.client->CreateToken(kTestTokenName, std::move(permissions));
  REQUIRE(err == Error::kOk);
  REQUIRE(token.starts_with("test_token-"));

  SECTION("Conflict") {
    REQUIRE(ctx.client->CreateToken(kTestTokenName, {}).error == Error{409, "Token 'test_token' already exists"});
  }
}

TEST_CASE("reduct::Client should get token", "[token_api]") {
  Fixture ctx;
  IClient::Permissions permissions{
      .full_access = true,
      .read = {"test_bucket_1"},
      .write = {"test_bucket_2"},
  };

  {
    auto [_, err] = ctx.client->CreateToken(kTestTokenName, permissions);
    REQUIRE(err == Error::kOk);
  }

  auto [token, err] = ctx.client->GetToken(kTestTokenName);
  REQUIRE(err == Error::kOk);
  REQUIRE(token.name == kTestTokenName);
  REQUIRE(token.created_at.time_since_epoch().count() > 0);
  REQUIRE(token.is_provisioned == false);
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
    auto [_, err] = ctx.client->CreateToken(kTestTokenName, {});
    REQUIRE(err == Error::kOk);
  }

  auto err = ctx.client->RemoveToken(kTestTokenName);
  REQUIRE(err == Error::kOk);

  SECTION("not found") {
    REQUIRE(ctx.client->RemoveToken("not-found") == Error{404, "Token 'not-found' doesn't exist"});
  }
}
