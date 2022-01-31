// Copyright 2022 Alexey Timin

#include "reduct/client.h"

#include <catch2/catch.hpp>

using reduct::IClient;

TEST_CASE("reduct::Client should get info", "[server_api]") {
  std::system("docker run -p 8383:8383 -d --rm ghcr.io/reduct-storage/reduct-storage:latest");
  auto client = IClient::Build("http://127.0.0.1:8383");
  //
  auto info = client->GetInfo();
  REQUIRE(info == IClient::ServerInfo{.version = "0.1.0", .bucket_count = 0});
}