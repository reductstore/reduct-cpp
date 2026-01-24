// Copyright 2022-2024 ReductSoftware UG

#include <catch2/catch.hpp>

#include <fstream>
#include <map>
#include <sstream>
#include <vector>

#include "fixture.h"
#include "reduct/client.h"
#include "reduct/internal/batch_v2.h"

using reduct::Error;
using reduct::IBucket;
using reduct::IClient;

const auto kBucketName = "test_bucket_3";
using us = std::chrono::microseconds;
using s = std::chrono::seconds;

TEST_CASE("reduct::IBucket should write/read a record", "[entry_api]") {
  Fixture ctx;
  auto [bucket, err] = ctx.client->CreateBucket(kBucketName);

  REQUIRE(err == Error::kOk);
  REQUIRE(bucket);

  IBucket::Time ts = IBucket::Time() + std::chrono::microseconds(123109210);
  std::string blob = "some blob of data";
  err = bucket->Write(
      "entry",
      IBucket::WriteOptions{
          .timestamp = ts, .labels = {{"label1", "value1"}, {"label2", "value2"}}, .content_type = "text/plain"},
      [&blob](auto rec) { rec->WriteAll(blob); });
  REQUIRE(err == Error::kOk);

  std::string received_data;
  err = bucket->Read("entry", ts, [&received_data, ts](auto record) {
    REQUIRE(record.size == 17);
    REQUIRE(record.timestamp == ts);
    REQUIRE(record.labels == std::map<std::string, std::string>{{"label1", "value1"}, {"label2", "value2"}});
    REQUIRE(record.content_type == "text/plain");

    auto [data, read_err] = record.ReadAll();
    received_data = std::move(data);
    REQUIRE(read_err == Error::kOk);
    return true;
  });

  REQUIRE(err == Error::kOk);
  REQUIRE(received_data == blob);

  SECTION("http errors") {
    REQUIRE(bucket->Read("entry", IBucket::Time(), [](auto) { return true; }) ==
            Error{.code = 404, .message = "No record with timestamp 0"});
  }
}

TEST_CASE("reduct::IBucket should read the latest record", "[entry_api]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);
  REQUIRE(bucket);

  using us = std::chrono::microseconds;
  IBucket::Time ts{};
  REQUIRE(bucket->Write("entry", ts, [](auto rec) { rec->WriteAll("some_data1"); }) == Error::kOk);
  REQUIRE(bucket->Write("entry", ts + us(1), [](auto rec) { rec->WriteAll("some_data2"); }) == Error::kOk);
  REQUIRE(bucket->Write("entry", ts + us(2), [](auto rec) { rec->WriteAll("some_data3"); }) == Error::kOk);

  auto err = bucket->Read("entry", {}, [ts](auto record) {
    REQUIRE(record.size == 10);
    REQUIRE(record.timestamp == ts + us(2));

    return true;
  });

  REQUIRE(err == Error::kOk);
}

TEST_CASE("reduct::IBucket should read a record in chunks", "[entry_api]") {
  Fixture ctx;
  auto [bucket, err] = ctx.client->CreateBucket(kBucketName);

  REQUIRE(err == Error::kOk);
  REQUIRE(bucket);

  IBucket::Time ts = IBucket::Time::clock::now();
  const std::string blob(10'000'000, 'x');
  REQUIRE(bucket->Write("entry", ts, [&blob](auto rec) { rec->WriteAll(blob); }) == Error::kOk);

  std::string received;
  REQUIRE(bucket->Read("entry", ts, [&received](auto record) {
    auto record_err = record.Read([&received](auto data) {
      received.append(data);
      return true;
    });

    REQUIRE(record_err == Error::kOk);
    return true;
  }) == Error::kOk);

  REQUIRE(received == blob);
}

TEST_CASE("reduct::IBucket should write a record in chunks", "[entry_api]") {
  Fixture ctx;
  auto [bucket, err] = ctx.client->CreateBucket(kBucketName);

  REQUIRE(err == Error::kOk);
  REQUIRE(bucket);

  IBucket::Time ts = IBucket::Time::clock::now();
  const std::string blob(10'000'000, 'x');
  REQUIRE(bucket->Write("entry", ts, [&blob](auto rec) {
    rec->Write(blob.size(), [&](auto offset, auto size) {
      return std::pair{
          true,
          blob.substr(offset, size),
      };
    });
  }) == Error::kOk);

  err = bucket->Read("entry", ts, [&blob](auto record) {
    auto [data, read_err] = record.ReadAll();
    REQUIRE(read_err == Error::kOk);
    REQUIRE(data == blob);

    return true;
  });

  REQUIRE(err == Error::kOk);
}

TEST_CASE("reduct::IBucket should query records", "[entry_api][1_13]") {
  auto [head, content] = GENERATE(std::make_tuple(false, "some_data1some_data2some_data3"), std::make_tuple(true, ""));
  CAPTURE(head);

  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);
  REQUIRE(bucket);

  IBucket::Time ts{};
  REQUIRE(bucket->Write("entry",
                        IBucket::WriteOptions{
                            .timestamp = ts,
                            .labels = {{"score", "10"}},
                        },
                        [](auto rec) { rec->WriteAll("some_data1"); }) == Error::kOk);
  REQUIRE(bucket->Write("entry", ts + us(1), [](auto rec) { rec->WriteAll("some_data2"); }) == Error::kOk);
  REQUIRE(bucket->Write("entry", ts + us(2), [](auto rec) { rec->WriteAll("some_data3"); }) == Error::kOk);

  std::string all_data;
  SECTION("receive all data") {
    auto err = bucket->Query("entry", ts, ts + us(3), {.head_only = head}, [&all_data](auto record) {
      auto read_err = record.Read([&all_data](auto data) {
        all_data.append(data);
        return true;
      });

      REQUIRE(read_err == Error::kOk);
      return true;
    });

    REQUIRE(err == Error::kOk);
    REQUIRE(all_data == content);
  }

  SECTION("stop receiving") {
    auto err = bucket->Query("entry", ts, ts + us(3), {}, [&all_data](auto record) {
      auto read_err = record.Read([&all_data](auto data) {
        all_data.append(data);
        return true;
      });

      REQUIRE(read_err == Error::kOk);
      return false;
    });

    REQUIRE(err == Error::kOk);
    REQUIRE(all_data == "some_data1");
  }

  SECTION("meta data about record") {
    auto err = bucket->Query("entry", ts, ts + us(1), {}, [ts](auto record) {
      REQUIRE(record.timestamp == ts);
      REQUIRE(record.size == 10);
      return true;
    });

    REQUIRE(err == Error::kOk);
  }

  SECTION("with condition") {
    auto err = bucket->Query("entry", ts, ts + us(3), {.when = R"({"&score": {"$gt": 0}})"}, [&all_data](auto record) {
      auto read_err = record.Read([&all_data](auto data) {
        all_data.append(data);
        return true;
      });

      REQUIRE(read_err == Error::kOk);
      return true;
    });

    REQUIRE(err == Error::kOk);
    REQUIRE(all_data == "some_data1");
  }

  SECTION("with strict condition") {
    auto err = bucket->Query("entry", ts, ts + us(3),
                             {
                                 .when = R"({"&NOT_EXIST": {"$gt": 0}})",
                                 .strict = true,
                             },
                             [&all_data](auto record) {
                               auto read_err = record.Read([&all_data](auto data) {
                                 all_data.append(data);
                                 return true;
                               });

                               REQUIRE(read_err == Error::kOk);
                               return true;
                             });

    REQUIRE(err == Error{.code = 404, .message = "Reference 'NOT_EXIST' not found"});
  }

  SECTION("with non strict condition") {
    auto err = bucket->Query("entry", ts, ts + us(3),
                             {
                                 .when = R"({"&NOT_EXIST": {"$gt": 0}})",
                                 .strict = false,
                             },
                             [&all_data](auto record) {
                               auto read_err = record.Read([&all_data](auto data) {
                                 all_data.append(data);
                                 return true;
                               });

                               REQUIRE(read_err == Error::kOk);
                               return true;
                             });

    REQUIRE(err == Error::kOk);
    REQUIRE(all_data.empty());
  }

  SECTION("order") {
    auto err = bucket->Query("entry", ts, ts + us(3),
                             {
                                 .when = R"({"$not": [ {"$has": "score"} ], "$limit": 2})",
                                 .strict = false,
                             },
                             [&all_data](auto record) {
                               auto read_err = record.Read([&all_data](auto data) {
                                 all_data.append(data);
                                 return true;
                               });

                               REQUIRE(read_err == Error::kOk);
                               return true;
                             });

    REQUIRE(err == Error::kOk);
    REQUIRE(all_data == "some_data2some_data3");
  }
}

TEST_CASE("reduct::IBucket should query records (huge blobs)", "[entry_api]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);
  REQUIRE(bucket);

  auto size = GENERATE(10, 100, 10'000, 1'000'000);
  CAPTURE(size);

  using us = std::chrono::microseconds;
  IBucket::Time ts{};
  std::string blob1(size, 'x');
  REQUIRE(bucket->Write("entry", ts, [&blob1](auto rec) { rec->WriteAll(blob1); }) == Error::kOk);

  std::string blob2(size - 7, 'y');
  REQUIRE(bucket->Write("entry", ts + us(1), [&blob2](auto rec) { rec->WriteAll(blob2); }) == Error::kOk);

  std::vector<std::string> received_data;
  auto err = bucket->Query("entry", ts, ts + us(3), {}, [&received_data](auto record) {
    auto [data, read_err] = record.ReadAll();

    received_data.push_back(data);
    REQUIRE(read_err == Error::kOk);
    return true;
  });

  REQUIRE(err == Error::kOk);
  REQUIRE(received_data[0] == blob1);
  REQUIRE(received_data[1] == blob2);
}

TEST_CASE("reduct::IBucket should query multiple entries", "[entry_api][1_18]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);
  REQUIRE(bucket);

  IBucket::Time ts{};
  REQUIRE(bucket->Write("entry-a", ts, [](auto rec) { rec->WriteAll("aaa"); }) == Error::kOk);
  REQUIRE(bucket->Write("entry-b", ts + us(1), [](auto rec) { rec->WriteAll("bbb"); }) == Error::kOk);

  std::map<std::string, std::string> received;
  auto err =
      bucket->Query(std::vector<std::string>{"entry-a", "entry-b"}, ts, ts + us(2), {}, [&received](auto record) {
        auto [data, read_err] = record.ReadAll();
        REQUIRE(read_err == Error::kOk);
        REQUIRE(!record.entry.empty());
        received[record.entry] = data;
        return true;
      });

  REQUIRE(err == Error::kOk);
  REQUIRE(received == std::map<std::string, std::string>{{"entry-a", "aaa"}, {"entry-b", "bbb"}});
}

TEST_CASE("reduct::internal::ParseAndBuildBatchedRecordsV2 should handle empty batch", "[entry_api][1_18]") {
  // Test the parsing function directly with empty entries header
  std::deque<std::optional<std::string>> data;
  std::mutex mutex;

  // Simulate empty batch response with empty entries header
  reduct::internal::IHttpClient::Headers headers;
  headers["x-reduct-entries"] = "";  // Empty entries
  headers["x-reduct-start-ts"] = "0";
  headers["x-reduct-last"] = "true";

  auto records = reduct::internal::ParseAndBuildBatchedRecordsV2(&data, &mutex, false, std::move(headers));

  REQUIRE(records.empty());  // Should return empty records, not crash
}

TEST_CASE("reduct::IBucket should reject empty entry list for batch query", "[entry_api][1_18]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);
  REQUIRE(bucket);

  auto err = bucket->Query(std::vector<std::string>{}, std::nullopt, std::nullopt, {}, [](auto) { return true; });

  REQUIRE(err.code == -1);
}

TEST_CASE("reduct::IBucket should update a batch across entries", "[entry_api][1_18]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);
  REQUIRE(bucket);

  IBucket::Time ts{};
  REQUIRE(bucket->Write("entry-a", ts, [](auto rec) { rec->WriteAll("aaa"); }) == Error::kOk);
  REQUIRE(bucket->Write("entry-b", ts, [](auto rec) { rec->WriteAll("bbb"); }) == Error::kOk);

  auto [errors, err] = bucket->UpdateBatch("entry-a", [ts](IBucket::Batch* batch) {
    batch->AddOnlyLabels("entry-a", ts, {{"x", "1"}});
    batch->AddOnlyLabels("entry-b", ts, {{"y", "2"}});
  });

  REQUIRE(err == Error::kOk);
  REQUIRE(errors.empty());

  REQUIRE(bucket->Read("entry-a", ts, [](auto record) {
    REQUIRE(record.labels == std::map<std::string, std::string>{{"x", "1"}});
    return true;
  }) == Error::kOk);

  REQUIRE(bucket->Read("entry-b", ts, [](auto record) {
    REQUIRE(record.labels == std::map<std::string, std::string>{{"y", "2"}});
    return true;
  }) == Error::kOk);
}

TEST_CASE("reduct::IBucket should remove records across entries via query", "[entry_api][1_18]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);
  REQUIRE(bucket);

  IBucket::Time ts{};
  REQUIRE(bucket->Write("entry-a", ts, [](auto rec) { rec->WriteAll("aaa"); }) == Error::kOk);
  REQUIRE(bucket->Write("entry-b", ts + us(1), [](auto rec) { rec->WriteAll("bbb"); }) == Error::kOk);

  auto [removed, err] = bucket->RemoveQuery(std::vector<std::string>{"entry-a", "entry-b"}, ts, ts + us(2), {});
  REQUIRE(err == Error::kOk);
  REQUIRE(removed == 2);

  REQUIRE(bucket->Read("entry-a", ts, [](auto) { return true; }) ==
          Error{.code = 404, .message = "No record with timestamp 0"});
  REQUIRE(bucket->Read("entry-b", ts + us(1), [](auto) { return true; }) ==
          Error{.code = 404, .message = "No record with timestamp 1"});
}

TEST_CASE("reduct::IBucket should reject empty entry list for batch remove query", "[entry_api][1_18]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);
  REQUIRE(bucket);

  auto [removed, err] = bucket->RemoveQuery(std::vector<std::string>{}, std::nullopt, std::nullopt, {});

  REQUIRE(removed == 0);
  REQUIRE(err.code == -1);
}

TEST_CASE("reduct::IBucket should remove a batch across entries", "[entry_api][1_18]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);
  REQUIRE(bucket);

  IBucket::Time ts{};
  REQUIRE(bucket->Write("entry-a", ts, [](auto rec) { rec->WriteAll("aaa"); }) == Error::kOk);
  REQUIRE(bucket->Write("entry-b", ts + us(1), [](auto rec) { rec->WriteAll("bbb"); }) == Error::kOk);

  auto [errors, err] = bucket->RemoveBatch("entry-a", [ts](IBucket::Batch* batch) {
    batch->AddRecord("entry-a", ts);
    batch->AddRecord("entry-b", ts + us(1));
  });

  REQUIRE(err == Error::kOk);
  REQUIRE(errors.empty());

  REQUIRE(bucket->Read("entry-a", ts, [](auto) { return true; }) ==
          Error{.code = 404, .message = "No record with timestamp 0"});
  REQUIRE(bucket->Read("entry-b", ts + us(1), [](auto) { return true; }) ==
          Error{.code = 404, .message = "No record with timestamp 1"});
}

TEST_CASE("reduct::IBucket should query data with ext parameter", "[bucket_api][1_15]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->GetBucket("test_bucket_1");
  REQUIRE(bucket);

  auto err = bucket->Query("entry-1", IBucket::Time{}, IBucket::Time::clock::now(), {.ext = R"({"test": {}})"},
                           [](auto record) { return true; });
  REQUIRE(err.message.starts_with("Unknown extension"));
}

TEST_CASE("reduct::IBucket should write batch of records", "[bucket_api][1_7]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);

  auto t = IBucket::Time();
  REQUIRE(bucket
              ->WriteBatch("entry-1",
                           [t](IBucket::Batch* batch) {
                             batch->AddRecord(t, "some_data1");
                             batch->AddRecord(t + us(1), "some_data2", "text/plain");
                             batch->AddRecord(t + us(2), "some_data3", "text/plain",
                                              {{"key1", "value1"}, {"key2", "value2"}});
                           })
              .error == Error::kOk);

  REQUIRE(bucket->Read("entry-1", t, [](auto record) {
    auto [data, err] = record.ReadAll();
    REQUIRE(err == Error::kOk);
    REQUIRE(data == "some_data1");
    REQUIRE(record.content_type == "application/octet-stream");
    REQUIRE(record.labels.empty());
    return true;
  }) == Error::kOk);

  REQUIRE(bucket->Read("entry-1", t + us(1), [](auto record) {
    auto [data, err] = record.ReadAll();
    REQUIRE(err == Error::kOk);
    REQUIRE(data == "some_data2");
    REQUIRE(record.content_type == "text/plain");
    REQUIRE(record.labels.empty());
    return true;
  }) == Error::kOk);

  REQUIRE(bucket->Read("entry-1", t + us(2), [](auto record) {
    auto [data, err] = record.ReadAll();
    REQUIRE(err == Error::kOk);
    REQUIRE(data == "some_data3");
    REQUIRE(record.content_type == "text/plain");
    REQUIRE(record.labels == std::map<std::string, std::string>{{"key1", "value1"}, {"key2", "value2"}});
    return true;
  }) == Error::kOk);
}

TEST_CASE("reduct::IBucket should write batch of records with errors", "[bucket_api][1_7]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);

  auto t = IBucket::Time();

  REQUIRE(bucket->Write("entry-1", t, [](auto rec) { rec->WriteAll("some_data1"); }) == Error::kOk);
  auto [record_errors, http_error] = bucket->WriteBatch("entry-1", [t](IBucket::Batch* batch) {
    batch->AddRecord(t, "some_data1");
    batch->AddRecord(t + us(1), "some_data2", "text/plain");
    batch->AddRecord(t + us(2), "some_data3", "text/plain", {{"key1", "value1"}, {"key2", "value2"}});
  });

  REQUIRE(http_error == Error::kOk);
  REQUIRE(record_errors.size() == 1);
  REQUIRE(record_errors[t].code == 409);
}

TEST_CASE("reduct::IBucket should update labels", "[bucket_api][1_11]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);

  auto t = IBucket::Time();
  REQUIRE(bucket->Write("entry-1",
                        {
                            .timestamp = t,
                            .labels = {{"key1", "value1"}, {"key2", "value2"}},
                        },
                        [](auto rec) { rec->WriteAll("some_data1"); }) == Error::kOk);

  REQUIRE(bucket->Update("entry-1", {
                                        .timestamp = t,
                                        .labels = {{"key1", "value1"}, {"key2", ""}},
                                    }) == Error::kOk);

  REQUIRE(bucket->Read("entry-1", t, [](auto record) {
    REQUIRE(record.labels == std::map<std::string, std::string>{{"key1", "value1"}});
    return true;
  }) == Error::kOk);
}

TEST_CASE("reduct::IBucket should update labels in barch and return errors", "[bucket_api][1_11]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);

  auto t = IBucket::Time();
  REQUIRE(bucket->Write("entry-1",
                        {
                            .timestamp = t,
                            .labels = {{"key1", "value1"}, {"key2", "value2"}},
                        },
                        [](auto rec) { rec->WriteAll("some_data1"); }) == Error::kOk);

  auto [record_errors, http_error] = bucket->UpdateBatch("entry-1", [t](IBucket::Batch* batch) {
    batch->AddOnlyLabels(t, {{"key1", "value1"}, {"key2", ""}});
    batch->AddOnlyLabels(t + us(1), {{"key1", "value1"}, {"key2", "value2"}});
  });

  REQUIRE(http_error == Error::kOk);
  REQUIRE(record_errors.size() == 1);
  REQUIRE(record_errors[t + us(1)].code == 404);
}

TEST_CASE("reduct::IBucket should remove a record", "[bucket_api][1_12]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);

  auto t = IBucket::Time();
  REQUIRE(bucket->Write("entry-1", t, [](auto rec) { rec->WriteAll("some_data1"); }) == Error::kOk);
  REQUIRE(bucket->Write("entry-1", t + us(1), [](auto rec) { rec->WriteAll("some_data2"); }) == Error::kOk);

  REQUIRE(bucket->RemoveRecord("entry-1", t) == Error::kOk);

  REQUIRE(bucket->Read("entry-1", t, [](auto record) { return true; }).code == 404);
  REQUIRE(bucket->Read("entry-1", t + us(1), [](auto record) { return true; }) == Error::kOk);
}

TEST_CASE("reduct::IBucket should remove a batch of records", "[bucket_api][1_12]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);

  auto t = IBucket::Time();
  REQUIRE(bucket->Write("entry-1", t, [](auto rec) { rec->WriteAll("some_data1"); }) == Error::kOk);
  REQUIRE(bucket->Write("entry-1", t + us(1), [](auto rec) { rec->WriteAll("some_data2"); }) == Error::kOk);

  auto [record_errors, http_error] = bucket->RemoveBatch("entry-1", [](IBucket::Batch* batch) {
    batch->AddRecord(IBucket::Time());
    batch->AddRecord(IBucket::Time() + us(1));
    batch->AddRecord(IBucket::Time() + us(100));
  });
  REQUIRE(http_error == Error::kOk);
  REQUIRE(record_errors.size() == 1);
  REQUIRE(record_errors[IBucket::Time() + us(100)].code == 404);

  REQUIRE(bucket->Read("entry-1", t, [](auto record) { return true; }) ==
          Error{.code = 404, .message = "No record with timestamp 0"});

  REQUIRE(bucket->Read("entry-1", t + us(1), [](auto record) {
    REQUIRE(record.size == 10);
    return true;
  }) == Error{.code = 404, .message = "No record with timestamp 1"});
}

TEST_CASE("reduct::IBucket should remove records by query", "[bucket_api][1_15]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);

  auto t = IBucket::Time();
  REQUIRE(bucket->Write("entry-1", t, [](auto rec) { rec->WriteAll("some_data1"); }) == Error::kOk);
  REQUIRE(bucket->Write("entry-1", t + us(1), [](auto rec) { rec->WriteAll("some_data2"); }) == Error::kOk);
  REQUIRE(bucket->Write("entry-1", t + us(2), [](auto rec) { rec->WriteAll("some_data3"); }) == Error::kOk);

  auto [removed_records, err] = bucket->RemoveQuery("entry-1", t, t + us(3), {.when = R"({"$each_n": 2})"});
  REQUIRE(err == Error::kOk);
  REQUIRE(removed_records == 1);

  REQUIRE(bucket->Read("entry-1", t + us(0), [](auto record) {
    REQUIRE(record.ReadAll().result == "some_data1");
    return true;
  }) == Error::kOk);

  REQUIRE(bucket->Read("entry-1", t + us(1), [](auto record) { return true; }).code == 404);

  REQUIRE(bucket->Read("entry-1", t + us(2), [](auto record) {
    REQUIRE(record.ReadAll().result == "some_data3");
    return true;
  }) == Error::kOk);
}

TEST_CASE("reduct::IBucket should remove records by query with when condition", "[bucket_api][1_13]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);

  auto t = IBucket::Time();
  REQUIRE(bucket->Write("entry-1", IBucket::WriteOptions{.timestamp = t, .labels = {{"score", "10"}}},
                        [](auto rec) { rec->WriteAll("some_data1"); }) == Error::kOk);
  REQUIRE(bucket->Write("entry-1", IBucket::WriteOptions{.timestamp = t + us(1), .labels = {{"score", "20"}}},
                        [](auto rec) { rec->WriteAll("some_data2"); }) == Error::kOk);

  SECTION("ok") {
    auto [removed_records, err] = bucket->RemoveQuery("entry-1", t, t + us(3), {.when = R"({"&score": {"$lt": 20}})"});
    REQUIRE(err == Error::kOk);
    REQUIRE(removed_records == 1);

    REQUIRE(bucket->Read("entry-1", t, [](auto record) { return true; }).code == 404);

    REQUIRE(bucket->Read("entry-1", t + us(1), [](auto record) {
      REQUIRE(record.ReadAll().result == "some_data2");
      return true;
    }) == Error::kOk);

    REQUIRE(bucket->Read("entry-1", t + us(2), [](auto record) { return true; }).code == 404);
  }

  SECTION("strict") {
    auto [removed_records, err] =
        bucket->RemoveQuery("entry-1", t, t + us(3), {.when = R"({"&NOT_EXIST": {"$lt": 20}})", .strict = true});
    REQUIRE(err == Error{.code = 404, .message = "Reference 'NOT_EXIST' not found"});
  }

  SECTION("non-strict") {
    auto [removed_records, err] =
        bucket->RemoveQuery("entry-1", t, t + us(3), {.when = R"({"&NOT_EXIST": {"$lt": 20}})", .strict = false});
    REQUIRE(err == Error::kOk);
    REQUIRE(removed_records == 0);
  }
}

TEST_CASE("reduct::IBucket should rename an entry", "[bucket_api][1_12]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);

  auto t = IBucket::Time();
  REQUIRE(bucket->Write("entry-1", t, [](auto rec) { rec->WriteAll("some_data1"); }) == Error::kOk);
  REQUIRE(bucket->RenameEntry("entry-1", "entry-new") == Error::kOk);

  REQUIRE(bucket->Read("entry-1", t, [](auto record) { return true; }) ==
          Error{.code = 404, .message = "Entry 'entry-1' not found in bucket 'test_bucket_3'"});

  REQUIRE(bucket->Read("entry-new", t, [](auto record) {
    REQUIRE(record.ReadAll().result == "some_data1");
    return true;
  }) == Error::kOk);
}

TEST_CASE("reduct::IBucket should write a batch to multiple entries", "[entry_api][1_18]") {
  Fixture ctx;
  auto [bucket, _] = ctx.client->CreateBucket(kBucketName);
  REQUIRE(bucket);

  auto t = IBucket::Time();
  auto [errors, err] = bucket->WriteBatch("default-entry", [t](IBucket::Batch* batch) {
    batch->AddRecord("entry-a", t, "aaa", "text/plain", {{"label", "one"}});
    batch->AddRecord("entry-b", t + us(1), "bbb");
  });

  REQUIRE(err == Error::kOk);
  REQUIRE(errors.empty());

  REQUIRE(bucket->Read("entry-a", t, [](auto record) {
    auto [data, read_err] = record.ReadAll();
    REQUIRE(read_err == Error::kOk);
    REQUIRE(data == "aaa");
    REQUIRE(record.labels == std::map<std::string, std::string>{{"label", "one"}});
    REQUIRE(record.content_type == "text/plain");
    return true;
  }) == Error::kOk);

  REQUIRE(bucket->Read("entry-b", t + us(1), [](auto record) {
    auto [data, read_err] = record.ReadAll();
    REQUIRE(read_err == Error::kOk);
    REQUIRE(data == "bbb");
    return true;
  }) == Error::kOk);
}

TEST_CASE("Batch should slice data", "[batch]") {
  auto batch = IBucket::Batch();

  batch.AddRecord(IBucket::Time(), "1111111111");
  batch.AddRecord(IBucket::Time() + us(1), "2222222222");
  batch.AddRecord(IBucket::Time() + us(2), "3333333333");

  REQUIRE(batch.size() == 30);
  REQUIRE(batch.records().size() == 3);

  SECTION("slice smaller record") {
    REQUIRE(batch.Slice(0, 6) == "111111");
    REQUIRE(batch.Slice(6, 6) == "111122");
    REQUIRE(batch.Slice(12, 6) == "222222");
    REQUIRE(batch.Slice(18, 6) == "223333");
    REQUIRE(batch.Slice(24, 6) == "333333");
  }

  SECTION("slice bigger record") {
    REQUIRE(batch.Slice(0, 15) == "111111111122222");
    REQUIRE(batch.Slice(15, 15) == "222223333333333");
  }

  SECTION("slice all record") { REQUIRE(batch.Slice(0, 30) == "111111111122222222223333333333"); }

  SECTION("slice out of range") { REQUIRE(batch.Slice(0, 31) == "111111111122222222223333333333"); }

  SECTION("slice with custom order") {
    std::vector<size_t> order{2, 0, 1};
    REQUIRE(batch.Slice(order, 0, 30) == "333333333311111111112222222222");
    REQUIRE(batch.Slice(order, 5, 10) == "3333311111");
  }
}
