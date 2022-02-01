// Copyright 2022 Alexey Timin

#include <catch2/catch.hpp>

#include "helpers.h"
#include "reduct/client.h"

using reduct::Error;
using reduct::IClient;

TEST_CASE("reduct::Client should get info", "[server_api]") {
  auto client = IClient::Build("http://127.0.0.1:8383");
  auto [info, err] = client->GetInfo();

  REQUIRE(err == Error::kOk);
  REQUIRE(info.version == "0.1.0");
}

TEST_CASE("reduct::Client should return error", "[server_api]") {
  auto client = IClient::Build("http://127.0.0.1:8382");
  auto [info, err] = client->GetInfo();

  REQUIRE(err == Error{.code = -1, .message = "Connection"});
}
