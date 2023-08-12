// Copyright 2022 Alexey Timin

#include <catch2/catch.hpp>

#include "fixture.h"
#include "reduct/client.h"

using reduct::Error;
using reduct::IBucket;
using reduct::IClient;

using s = std::chrono::seconds;

constexpr auto kBucketName = "bucket";

TEST_CASE("reduct::Client should create a bucket", "[bucket_api]") {
  Fixture ctx;
  auto [bucket, err] = ctx.client->CreateBucket(kBucketName);

  REQUIRE(err == Error::kOk);
  REQUIRE(bucket);

  SECTION("should return HTTP status") {
    auto [null, err_409] = ctx.client->CreateBucket(kBucketName);
    REQUIRE(err_409 == Error{.code = 409, .message = fmt::format("Bucket '{}' already exists", kBucketName)});
    REQUIRE_FALSE(null);
  }
}

TEST_CASE("reduct::Client should get a bucket", "[bucket_api]") {
  Fixture ctx;
  [[maybe_unused]] auto _ = ctx.client->CreateBucket(kBucketName);
  auto [bucket, err] = ctx.client->GetBucket(kBucketName);

  REQUIRE(err == Error::kOk);
  REQUIRE(bucket);

  SECTION("should return HTTP status") {
    auto [null, err_404] = ctx.client->GetBucket("XXXXXX");
    REQUIRE(err_404 == Error{.code = 404, .message = "Bucket 'XXXXXX' is not found"});
    REQUIRE_FALSE(null);
  }
}

TEST_CASE("reduct::Client should get or create a bucket", "[bucket_api]") {
  Fixture ctx;
  [[maybe_unused]] auto _ = ctx.client->GetOrCreateBucket(kBucketName);
  auto [bucket, err] = ctx.client->GetOrCreateBucket(kBucketName);

  REQUIRE(err == Error::kOk);
  REQUIRE(bucket);
}

TEST_CASE("reduct::IBucket should have settings", "[bucket_api]") {
  Fixture ctx;
  const IBucket::Settings kSettings{
      .max_block_size = 100,
      .quota_type = IBucket::QuotaType::kFifo,
      .quota_size = 1000,
      .max_block_records = 200,
  };
  auto [bucket, create_err] = ctx.client->CreateBucket(kBucketName, kSettings);

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
          .max_block_records = 1020,
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
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);

  auto t = IBucket::Time();
  REQUIRE(bucket->Write("entry-1", t, [](auto record) { record->WriteAll("some_data"); }) == Error::kOk);
  REQUIRE(bucket->Write("entry-2", t + std::chrono::seconds(1), [](auto record) { record->WriteAll("some_data"); }) ==
          Error::kOk);

  auto [info, err] = bucket->GetInfo();
  REQUIRE(err == Error::kOk);

  REQUIRE(info == IBucket::BucketInfo{
                      .name = kBucketName,
                      .entry_count = 2,
                      .size = 18,
                      .oldest_record = t,
                      .latest_record = t + std::chrono::seconds(1),
                  });
}

TEST_CASE("reduct::IBucket should get list of entries", "[bucket_api]") {
  Fixture ctx;

  auto [entries, err] = ctx.bucket_1->GetEntryList();
  REQUIRE(err == Error::kOk);
  REQUIRE(entries.size() == 2);

  auto t = IBucket::Time();
  REQUIRE(entries[0] == IBucket::EntryInfo{
                            .name = "entry-1",
                            .record_count = 2,
                            .block_count = 1,
                            .size = 12,
                            .oldest_record = t + s(1),
                            .latest_record = t + s(2),
                        });

  REQUIRE(entries[1] == IBucket::EntryInfo{
                            .name = "entry-2",
                            .record_count = 2,
                            .block_count = 1,
                            .size = 12,
                            .oldest_record = t + s(3),
                            .latest_record = t + s(4),
                        });
}

TEST_CASE("reduct::IBucket should remove a bucket", "[bucket_api]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);

  REQUIRE(bucket->Remove() == Error::kOk);
  REQUIRE(bucket->Remove() == Error{.code = 404, .message = fmt::format("Bucket '{}' is not found", kBucketName)});
}

TEST_CASE("reduct::IBucket should remove entry", "[bucket_api][1_6]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);

  auto t = IBucket::Time();
  REQUIRE(bucket->Write("entry-1", t, [](auto record) { record->WriteAll("some_data"); }) == Error::kOk);
  REQUIRE(bucket->Write("entry-2", t + std::chrono::seconds(1), [](auto record) { record->WriteAll("some_data"); }) ==
          Error::kOk);

  REQUIRE(bucket->RemoveEntry("entry-1") == Error::kOk);
  REQUIRE(bucket->RemoveEntry("entry-1") ==
          Error{.code = 404, .message = fmt::format("Entry 'entry-1' not found in bucket '{}'", kBucketName)});
}
