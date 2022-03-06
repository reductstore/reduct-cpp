// Copyright 2022 Alexey Timin

#include <catch2/catch.hpp>

#include "helpers.h"
#include "reduct/client.h"

using reduct::Error;
using reduct::IBucket;
using reduct::IClient;

TEST_CASE("reduct::Client should create a bucket", "[bucket_api]") {
  IClient::Options opts;
  auto token =  std::getenv("REDUCT_CPP_TOKEN_API");
  if (token != nullptr) {
    opts.api_token = token;
  };
  auto client = IClient::Build("http://127.0.0.1:8383", opts);

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
  auto client = IClient::Build("http://127.0.0.1:8383");

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
  auto client = IClient::Build("http://127.0.0.1:8383");

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

TEST_CASE("reduct::IBucket should remove a bucket", "[bucket_api") {
  auto client = IClient::Build("http://127.0.0.1:8383");

  const auto kBucketName = RandomBucketName();
  auto [bucket, _] = client->CreateBucket(kBucketName);

  REQUIRE(bucket->Remove() == Error::kOk);
  REQUIRE(bucket->Remove() == Error{.code = 404, .message = fmt::format("Bucket '{}' is not found", kBucketName)});
}
