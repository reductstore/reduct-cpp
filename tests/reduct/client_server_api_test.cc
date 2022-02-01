// Copyright 2022 Alexey Timin

#include "reduct/client.h"

#include <catch2/catch.hpp>

using reduct::Error;
using reduct::IClient;

struct StorageLauncher {
  StorageLauncher() {
    std::system(
        "docker run -p 8383:8383 --rm -d --name=reduct-cpp-container ghcr.io/reduct-storage/reduct-storage:main");
  }

  ~StorageLauncher() { std::system("docker stop reduct-cpp-container"); }
};

TEST_CASE("reduct::Client should get info", "[server_api]") {
  StorageLauncher launcher;

  auto client = IClient::Build("http://127.0.0.1:8383");
  //
  auto [info, err] = client->GetInfo();
  REQUIRE(err == Error::kOk);
  REQUIRE(info == IClient::ServerInfo{.version = "0.1.0", .bucket_count = 0});
}

TEST_CASE("reduct::Client should return error", "[server_api]") {
  auto client = IClient::Build("http://127.0.0.1:8383");
  //
  auto [info, err] = client->GetInfo();
  REQUIRE(err == Error{.code = -1, .message = "Connection"});
}