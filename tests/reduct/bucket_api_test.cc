// Copyright 2022-2025 ReductSoftware UG

#include <catch2/catch.hpp>

#include <vector>

#include "fixture.h"
#include "reduct/client.h"
#include "reduct/internal/http_client.h"

using reduct::Error;
using reduct::IBucket;
using reduct::IClient;
using reduct::Result;

using s = std::chrono::seconds;

constexpr auto kBucketName = "test_bucket_3";

TEST_CASE("reduct::Client should create a bucket", "[bucket_api]") {
  Fixture ctx;
  auto [bucket, err] = ctx.client->CreateBucket(kBucketName);

  REQUIRE(err == Error::kOk);
  REQUIRE(bucket);

  SECTION("should return HTTP status") {
    auto [null, err_409] = ctx.client->CreateBucket(kBucketName);
    REQUIRE(err_409.code == 409);
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
    REQUIRE(err_404.code == 404);
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

TEST_CASE("reduct::Client should create a bucket with different quota type", "[bucket_api][1_12]") {
  Fixture ctx;

  auto quota_type = GENERATE(IBucket::QuotaType::kNone, IBucket::QuotaType::kFifo, IBucket::QuotaType::kHard);
  [[maybe_unused]] auto _ = ctx.client->GetOrCreateBucket(kBucketName, {.quota_type = quota_type});
  auto [bucket, err] = ctx.client->GetOrCreateBucket(kBucketName);

  REQUIRE(err == Error::kOk);
  REQUIRE(bucket);

  auto [settings, get_error] = bucket->GetSettings();
  REQUIRE(get_error == Error::kOk);
  REQUIRE(settings.quota_type == quota_type);
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
      std::this_thread::sleep_for(std::chrono::milliseconds(100));  // wait for bucket deletion
      auto get_err = bucket->GetSettings();
      REQUIRE(get_err.error.code == 404);
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
                      .size = 80,
                      .oldest_record = t,
                      .latest_record = t + std::chrono::seconds(1),
                      .is_provisioned = false,
                      .status = IBucket::Status::kReady,
                  });
}

TEST_CASE("reduct::IBucket should get list of entries", "[bucket_api]") {
  Fixture ctx;

  auto [entries, err] = ctx.test_bucket_1->GetEntryList();
  REQUIRE(err == Error::kOk);
  REQUIRE(entries.size() == 2);

  auto t = IBucket::Time();
  REQUIRE(entries[0] == IBucket::EntryInfo{
                            .name = "entry-1",
                            .record_count = 2,
                            .block_count = 1,
                            .size = 78,
                            .oldest_record = t + s(1),
                            .latest_record = t + s(2),
                            .status = IBucket::Status::kReady,
                        });

  REQUIRE(entries[1] == IBucket::EntryInfo{
                            .name = "entry-2",
                            .record_count = 2,
                            .block_count = 1,
                            .size = 78,
                            .oldest_record = t + s(3),
                            .latest_record = t + s(4),
                            .status = IBucket::Status::kReady,
                        });
}

TEST_CASE("reduct::IBucket should remove a bucket", "[bucket_api]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);

  REQUIRE(bucket->Remove() == Error::kOk);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));  // wait for bucket deletion
  auto remove_err = bucket->Remove();
  REQUIRE(remove_err.code == 404);
}

TEST_CASE("reduct::IBucket should remove entry", "[bucket_api][1_6]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);

  auto t = IBucket::Time();
  REQUIRE(bucket->Write("entry-1", t, [](auto record) { record->WriteAll("some_data"); }) == Error::kOk);
  REQUIRE(bucket->Write("entry-2", t + std::chrono::seconds(1), [](auto record) { record->WriteAll("some_data"); }) ==
          Error::kOk);

  REQUIRE(bucket->RemoveEntry("entry-1") == Error::kOk);
  // After removal, the entry may be in DELETING state (409) or not found (404)
  auto err = bucket->RemoveEntry("entry-1");
  REQUIRE((err.code == 404 || err.code == 409));
}

TEST_CASE("reduct::IBucket should rename bucket", "[bucket_api][1_12]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);
  auto err = bucket->Rename("test_bucket_new");
  REQUIRE(err == Error::kOk);
  REQUIRE(bucket->GetInfo().result.name == "test_bucket_new");

  auto [bucket_old, get_err] = ctx.client->GetBucket(kBucketName);
  REQUIRE(get_err.code == 404);
}

Result<std::string> download_link(std::string_view link) {
  auto http_client = reduct::internal::IHttpClient::Build("http://127.0.0.1:8383", {});
  return http_client->Get(link.substr(link.find("/links/")));
}


TEST_CASE("reduct::IBucket should create a query link", "[bucket_api][1_17]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->GetBucket("test_bucket_1");

  auto [link, err] = bucket->CreateQueryLink("entry-1", IBucket::QueryLinkOptions{});
  REQUIRE(err == Error::kOk);

  auto [data, http_err] = download_link(link);
  REQUIRE(http_err == Error::kOk);
  REQUIRE(data == "data-1");
}

TEST_CASE("reduct::IBucket should create a query link for multiple entries", "[bucket_api][1_18]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->GetBucket("test_bucket_1");

  auto [link, err] = bucket->CreateQueryLink(std::vector<std::string>{"entry-1", "entry-2"},
                                             IBucket::QueryLinkOptions{});
  REQUIRE(err == Error::kOk);

  auto [data, http_err] = download_link(link);
  REQUIRE(http_err == Error::kOk);
  REQUIRE(data == "data-1");
}

TEST_CASE("reduct::IBucket should reject empty entry list for query link", "[bucket_api][1_18]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->GetBucket("test_bucket_1");

  auto [link, err] = bucket->CreateQueryLink(std::vector<std::string>{}, IBucket::QueryLinkOptions{});
  REQUIRE(err.code == -1);
  REQUIRE(link.empty());
}

TEST_CASE("reduct::IBucket should create a query link with index", "[bucket_api][1_17]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->GetBucket("test_bucket_1");

  auto [link, err] = bucket->CreateQueryLink("entry-1", IBucket::QueryLinkOptions{.record_index = 1});
  REQUIRE(err == Error::kOk);

  auto [data, http_err] = download_link(link);
  REQUIRE(http_err == Error::kOk);
  REQUIRE(data == "data-2");
}

TEST_CASE("reduct::IBucket should create a query link with file name", "[bucket_api][1_17]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->GetBucket("test_bucket_1");

  auto [link, err] = bucket->CreateQueryLink("entry-1", IBucket::QueryLinkOptions{
                                                            .file_name = "my_file.txt",
                                                        });
  REQUIRE(err == Error::kOk);
  REQUIRE(link.find("/links/my_file.txt") != std::string::npos);
}

TEST_CASE("reduct::IBucket should create a query link with expire time", "[bucket_api][1_17]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->GetBucket("test_bucket_1");

  auto [link, err] =
      bucket->CreateQueryLink("entry-1", IBucket::QueryLinkOptions{
                                             .expire_at = IBucket::Time::clock::now() - std::chrono::hours(1),
                                         });
  REQUIRE(err == Error::kOk);

  auto [_data, http_err] = download_link(link);
  REQUIRE(http_err.code == 422);
}
