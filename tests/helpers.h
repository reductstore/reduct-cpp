// Copyright 2022 Alexey Timin

#ifndef REDUCT_CPP_HELPERS_H
#define REDUCT_CPP_HELPERS_H

#include <fmt/core.h>

#include <cstdlib>
#include <random>

#include "reduct/client.h"

inline std::string RandomBucketName() {
  std::random_device r;

  std::default_random_engine e1(r());
  std::uniform_int_distribution<int> uniform_dist(1, 10000000);

  return fmt::format("bucket_{}", uniform_dist(e1));
}

inline std::unique_ptr<reduct::IClient> BuildClient(std::string_view url = "http://127.0.0.1:8383") {
  using reduct::IClient;

  IClient::Options opts;
  auto token = std::getenv("REDUCT_CPP_TOKEN_API");
  if (token != nullptr) {
    opts.api_token = token;
  }

  return IClient::Build(url, opts);
}

#endif  // REDUCT_CPP_HELPERS_H
