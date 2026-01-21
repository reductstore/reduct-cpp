// Copyright 2022-2026 ReductSoftware UG

#ifndef REDUCT_CPP_HELPERS_H
#define REDUCT_CPP_HELPERS_H

#include <fmt/core.h>

#include <cstdlib>
#include <random>
#include <thread>

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
    auto bucket_list = client->GetBucketList();
    if (bucket_list.error) {
      throw std::runtime_error(fmt::format("Failed to get bucket list: {}", bucket_list.error.ToString()));
    }
    for (auto&& info : client->GetBucketList().result) {
      if (!info.name.starts_with("test_bucket")) {
        continue;
      }
      std::unique_ptr<IBucket> bucket = client->GetBucket(info.name);
      [[maybe_unused]] auto ret = bucket->Remove();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // wait for bucket removal

    for (auto&& t : client->GetTokenList().result) {
      if (!t.name.starts_with("test_token")) {
        continue;
      }
      [[maybe_unused]] auto ret = client->RemoveToken(t.name);
    }

    for (auto&& r : client->GetReplicationList().result) {
      if (!r.name.starts_with("test_replication")) {
        continue;
      }
      [[maybe_unused]] auto ret = client->RemoveReplication(r.name);
    }

    auto [bucket, err] = client->CreateBucket("test_bucket_1");
    if (err != reduct::Error::kOk) {
      throw std::runtime_error(fmt::format("Failed to create test_bucket_1: {}", err.ToString()));
    }
    test_bucket_1 = std::move(bucket);
    [[maybe_unused]] auto ret =
        test_bucket_1->Write("entry-1", IBucket::Time() + s(1), [](auto rec) { rec->WriteAll("data-1"); });
    ret = test_bucket_1->Write("entry-1", IBucket::Time() + s(2), [](auto rec) { rec->WriteAll("data-2"); });
    ret = test_bucket_1->Write("entry-2", IBucket::Time() + s(3), [](auto rec) { rec->WriteAll("data-3"); });
    ret = test_bucket_1->Write("entry-2", IBucket::Time() + s(4), [](auto rec) { rec->WriteAll("data-4"); });

    test_bucket_2 = client->CreateBucket("test_bucket_2").result;
    ret = test_bucket_2->Write("entry-1", IBucket::Time() + s(5), [](auto rec) { rec->WriteAll("data-5"); });
    ret = test_bucket_2->Write("entry-1", IBucket::Time() + s(6), [](auto rec) { rec->WriteAll("data-6"); });
  }

  std::unique_ptr<reduct::IClient> client;
  std::unique_ptr<reduct::IBucket> test_bucket_1;
  std::unique_ptr<reduct::IBucket> test_bucket_2;
};

#endif  // REDUCT_CPP_HELPERS_H
