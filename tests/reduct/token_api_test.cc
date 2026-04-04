// Copyright 2022-2024 ReductSoftware UG

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
  // REQUIRE_FALSE(tokens[0].is_provisioned); TODO: uncomment in next release
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
    REQUIRE(ctx.client->CreateToken(kTestTokenName, IClient::Permissions{}).error == Error{409, "Token 'test_token' already exists"});
  }
}

TEST_CASE("reduct::Client should create token with ttl and ip allowlist", "[token_api][1_19]") {
  Fixture ctx;
  IClient::TokenCreateRequest request{
      .permissions = {.full_access = false, .read = {"test_bucket_1"}, .write = {"test_bucket_2"}},
      .ttl = 3600,
      .ip_allowlist = {"127.0.0.1", "10.10.0.0/16"},
  };

  auto [token, err] = ctx.client->CreateToken(kTestTokenName, request);
  REQUIRE(err == Error::kOk);
  REQUIRE(token.starts_with("test_token-"));

  auto [info, info_err] = ctx.client->GetToken(kTestTokenName);
  REQUIRE(info_err == Error::kOk);
  REQUIRE(info.ttl == request.ttl);
  REQUIRE(info.ip_allowlist == request.ip_allowlist);
  REQUIRE_FALSE(info.is_expired);
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
  REQUIRE_FALSE(token.expires_at.has_value());
  REQUIRE_FALSE(token.ttl.has_value());
  REQUIRE(token.ip_allowlist.empty());
  REQUIRE_FALSE(token.is_expired);

  SECTION("not found") {
    REQUIRE(ctx.client->GetToken("not-found").error == Error{404, "Token 'not-found' doesn't exist"});
  }
}

TEST_CASE("reduct::Client should rotate token", "[token_api][1_19]") {
  Fixture ctx;
  auto [token, err] = ctx.client->CreateToken(kTestTokenName, IClient::Permissions{});
  REQUIRE(err == Error::kOk);

  auto [rotated_token, rotate_err] = ctx.client->RotateToken(kTestTokenName);
  REQUIRE(rotate_err == Error::kOk);
  REQUIRE(rotated_token.starts_with("test_token-"));
  REQUIRE(rotated_token != token);

  reduct::HttpOptions old_opts{};
  old_opts.api_token = token;
  auto old_client = IClient::Build("http://127.0.0.1:8383", old_opts);
  REQUIRE(old_client->GetBucketList().error.code == 401);

  reduct::HttpOptions new_opts{};
  new_opts.api_token = rotated_token;
  auto new_client = IClient::Build("http://127.0.0.1:8383", new_opts);
  REQUIRE(new_client->GetBucketList().error == Error::kOk);
}

TEST_CASE("reduct::Client should delete token", "[token_api]") {
  Fixture ctx;
  {
    auto [_, err] = ctx.client->CreateToken(kTestTokenName, IClient::Permissions{});
    REQUIRE(err == Error::kOk);
  }

  auto err = ctx.client->RemoveToken(kTestTokenName);
  REQUIRE(err == Error::kOk);

  SECTION("not found") {
    REQUIRE(ctx.client->RemoveToken("not-found") == Error{404, "Token 'not-found' doesn't exist"});
  }
}
