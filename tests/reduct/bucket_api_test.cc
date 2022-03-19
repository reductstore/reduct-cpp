// Copyright 2022 Alexey Timin

#include <catch2/catch.hpp>

#include "helpers.h"
#include "reduct/client.h"

using reduct::Error;
using reduct::IBucket;
using reduct::IClient;

TEST_CASE("reduct::Client should create a bucket", "[bucket_api]") {
  auto client = BuildClient();
  const auto kBucketName = RandomBucketName();
  auto [bucket, err] = client->CreateBucket(kBucketName);

  REQUIRE(err == Error::kOk);
  REQUIRE(bucket);

  SECTION("should return HTTP status") {
    auto [null, err_409] = client->CreateBucket(kBucketName);
    REQUIRE(err_409 == Error{.code = 409, .message = fmt::format("Bucket '{}' already exists", kBucketName)});
    REQUIRE_FALSE(null);
  }
}

TEST_CASE("reduct::Client should get a bucket", "[bucket_api]") {
  auto client = BuildClient();
  const auto kBucketName = RandomBucketName();
  [[maybe_unused]] auto _ = client->CreateBucket(kBucketName);

  auto [bucket, err] = client->GetBucket(kBucketName);

  REQUIRE(err == Error::kOk);
  REQUIRE(bucket);

  SECTION("should return HTTP status") {
    auto [null, err_404] = client->GetBucket("XXXXXX");
    REQUIRE(err_404 == Error{.code = 404, .message = "Bucket 'XXXXXX' is not found"});
    REQUIRE_FALSE(null);
  }
}

TEST_CASE("reduct::IBucket should have settings", "[bucket_api]") {
  auto client = BuildClient();
  const auto kBucketName = RandomBucketName();
  const IBucket::Settings kSettings{
      .max_block_size = 100,
      .quota_type = IBucket::QuotaType::kFifo,
      .quota_size = 1000,
  };
  auto [bucket, create_err] = client->CreateBucket(kBucketName, kSettings);

  REQUIRE(create_err == Error::kOk);
  REQUIRE(bucket);

  SECTION("get settings") {
    auto [settings, get_error] = bucket->GetSettings();
    REQUIRE(get_error == Error::kOk);
    REQUIRE(settings == kSettings);

    SECTION("and return HTTP status") {
      REQUIRE(bucket->Remove() == Error::kOk);
      REQUIRE(bucket->GetSettings() ==
              Error{.code = 404, .message = fmt::format("Bucket '{}' is not found", kBucketName)});
    }
  }

  SECTION("set settings") {
    SECTION("partial") {
      auto err = bucket->UpdateSettings(IBucket::Settings{.quota_size = 999});
      REQUIRE(err == Error::kOk);

      auto [settings, get_error] = bucket->GetSettings();
      REQUIRE(settings.quota_size == 999);
    }

    SECTION("full") {
      const IBucket::Settings kNewSettings{
          .max_block_size = 777777,
          .quota_type = IBucket::QuotaType::kNone,
          .quota_size = 999,
      };
      auto err = bucket->UpdateSettings(kNewSettings);
      REQUIRE(err == Error::kOk);

      auto [settings, get_error] = bucket->GetSettings();
      REQUIRE(kNewSettings == settings);
    }

    SECTION("and return HTTP status") {
      REQUIRE(bucket->Remove() == Error::kOk);
      REQUIRE(bucket->GetSettings() ==
              Error{.code = 404, .message = fmt::format("Bucket '{}' is not found", kBucketName)});
    }
  }
}

TEST_CASE("reduct::IBucket should get bucket stats", "[bucket_api]") {
  auto client = BuildClient();

  const auto kBucketName = RandomBucketName();
  auto [bucket, _] = client->CreateBucket(kBucketName);

  auto t = IBucket::Time();
  REQUIRE(bucket->Write("entry-1", "some_data", t) == Error::kOk);
  REQUIRE(bucket->Write("entry-2", "some_data", t + std::chrono::seconds(1)) == Error::kOk);

  auto [info, err] = bucket->GetInfo();
  REQUIRE(err == Error::kOk);

  REQUIRE(info == IBucket::BucketInfo{
                      .name = kBucketName,
                      .entry_count = 2,
                      .size = 22,
                      .oldest_record = t,
                      .latest_record = t + std::chrono::seconds(1),
                  });
}

TEST_CASE("reduct::IBucket should get list of entries") {
  auto client = BuildClient();

  const auto kBucketName = RandomBucketName();
  auto [bucket, _] = client->CreateBucket(kBucketName);

  REQUIRE(bucket->Write("entry-1", "some_data") == Error::kOk);
  REQUIRE(bucket->Write("entry-2", "some_data") == Error::kOk);

  auto [entries, err] = bucket->GetEntryList();
  REQUIRE(entries == std::vector<std::string>{"entry-1", "entry-2"});
}

TEST_CASE("reduct::IBucket should remove a bucket", "[bucket_api") {
  auto client = BuildClient();

  const auto kBucketName = RandomBucketName();
  auto [bucket, _] = client->CreateBucket(kBucketName);

  REQUIRE(bucket->Remove() == Error::kOk);
  REQUIRE(bucket->Remove() == Error{.code = 404, .message = fmt::format("Bucket '{}' is not found", kBucketName)});
}
