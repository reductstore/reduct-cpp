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

    for (auto&& t : client->GetTokenList().result) {
      if (t.name != "init-token") {
        [[maybe_unused]] auto ret = client->RemoveToken(t.name);
      }
    }

    auto [bucket_1, err] = client->CreateBucket("bucket_1");
    if (err != reduct::Error::kOk) {
      throw std::runtime_error(fmt::format("Failed to create bucket_1: {}", err.ToString()));
    }
    [[maybe_unused]] auto ret =
        bucket_1->Write("entry-1", IBucket::Time() + s(1), [](auto rec) { rec->WriteAll("data-1"); });
    ret = bucket_1->Write("entry-1", IBucket::Time() + s(2), [](auto rec) { rec->WriteAll("data-2"); });
    ret = bucket_1->Write("entry-2", IBucket::Time() + s(3), [](auto rec) { rec->WriteAll("data-3"); });
    ret = bucket_1->Write("entry-2", IBucket::Time() + s(4), [](auto rec) { rec->WriteAll("data-4"); });

    bucket_2 = client->CreateBucket("bucket_2").result;
    ret = bucket_2->Write("entry-1", IBucket::Time() + s(5), [](auto rec) { rec->WriteAll("data-5"); });
    ret = bucket_2->Write("entry-1", IBucket::Time() + s(6), [](auto rec) { rec->WriteAll("data-6"); });
  }

  std::unique_ptr<reduct::IClient> client;
  std::unique_ptr<reduct::IBucket> bucket_1;
  std::unique_ptr<reduct::IBucket> bucket_2;
};

#endif  // REDUCT_CPP_HELPERS_H
