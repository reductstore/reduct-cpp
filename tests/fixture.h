// Copyright 2022 Alexey Timin

#ifndef REDUCT_CPP_HELPERS_H
#define REDUCT_CPP_HELPERS_H

#include <fmt/core.h>

#include <cstdlib>
#include <random>

#include "reduct/client.h"

struct Fixture {
  explicit Fixture(std::string_view url = "http://127.0.0.1:8383") {
    using reduct::IBucket;
    using reduct::IClient;
    using s = std::chrono::seconds;

    reduct::HttpOptions opts{};
    auto token = std::getenv("REDUCT_CPP_TOKEN_API");
    if (token != nullptr) {
      opts.api_token = token;
    }

    client = IClient::Build(url, opts);
    for (auto&& info : client->GetBucketList().result) {
      std::unique_ptr<IBucket> bucket = client->GetBucket(info.name);
      [[maybe_unused]] auto ret = bucket->Remove();
    }

    bucket_1 = client->CreateBucket("bucket_1").result;
    [[maybe_unused]] auto ret = bucket_1->Write("entry-1", "data-1", IBucket::Time() + s(1));
    ret = bucket_1->Write("entry-1", "data-2", IBucket::Time() + s(2));
    ret = bucket_1->Write("entry-2", "data-3", IBucket::Time() + s(3));
    ret = bucket_1->Write("entry-2", "data-4", IBucket::Time() + s(4));

    bucket_2 = client->CreateBucket("bucket_2").result;
    ret = bucket_2->Write("entry-1", "data-5", IBucket::Time() + s(5));
    ret = bucket_2->Write("entry-1", "data-6", IBucket::Time() + s(6));
  }

  std::unique_ptr<reduct::IClient> client;
  std::unique_ptr<reduct::IBucket> bucket_1;
  std::unique_ptr<reduct::IBucket> bucket_2;
};

#endif  // REDUCT_CPP_HELPERS_H
