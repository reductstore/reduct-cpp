// Copyright 2026 ReductSoftware UG

#include <catch2/catch.hpp>

#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "reduct/bucket.h"
#include "reduct/internal/bucket.h"
#include "reduct/internal/headers.h"

using reduct::Error;
using reduct::IBucket;
using reduct::Result;
using reduct::internal::IHttpClient;

namespace {

struct Request {
  std::string method;
  std::string path;
  IHttpClient::Headers headers;
};

class FakeHttpClient : public IHttpClient {
 public:
  explicit FakeHttpClient(std::string api_version) : api_version_(std::move(api_version)) {}

  Result<std::string> Get(std::string_view path) const noexcept override {
    Capture("GET", path, {});
    return {{}, Error::kOk};
  }

  Error Get(std::string_view path, ResponseCallback response_callback,
            ReadCallback read_callback) const noexcept override {
    return Get(path, {}, std::move(response_callback), std::move(read_callback));
  }

  Error Get(std::string_view path, Headers headers, ResponseCallback response_callback,
            ReadCallback read_callback) const noexcept override {
    Capture("GET", path, std::move(headers));
    response_callback({});
    read_callback({});
    return Error::kOk;
  }

  Result<Headers> Head(std::string_view path) const noexcept override { return Head(path, {}); }

  Result<Headers> Head(std::string_view path, Headers headers) const noexcept override {
    Capture("HEAD", path, std::move(headers));
    return {{}, Error::kOk};
  }

  Error Post(std::string_view path, std::string_view body, std::string_view mime) const noexcept override {
    (void)body;
    (void)mime;
    Capture("POST", path, {});
    return Error::kOk;
  }

  Result<std::string> PostWithResponse(std::string_view path, std::string_view body,
                                       std::string_view mime) const noexcept override {
    (void)body;
    (void)mime;
    Capture("POST", path, {});
    return {"{}", Error::kOk};
  }

  Result<std::tuple<std::string, Headers>> Post(std::string_view path, std::string_view mime, size_t content_length,
                                                Headers headers, WriteCallback callback) const noexcept override {
    (void)mime;
    Capture("POST", path, std::move(headers));
    callback(0, content_length);
    return {{"", {}}, Error::kOk};
  }

  Error Put(std::string_view path, std::string_view body, std::string_view mime) const noexcept override {
    (void)body;
    (void)mime;
    Capture("PUT", path, {});
    return Error::kOk;
  }

  Result<std::tuple<std::string, Headers>> Patch(std::string_view path, std::string_view body,
                                                 Headers headers) const noexcept override {
    (void)body;
    Capture("PATCH", path, std::move(headers));
    return {{"", {}}, Error::kOk};
  }

  Result<std::tuple<std::string, Headers>> Delete(std::string_view path, Headers headers) const noexcept override {
    Capture("DELETE", path, std::move(headers));
    return {{"", {}}, Error::kOk};
  }

  [[nodiscard]] std::optional<std::string> ApiVersion() const noexcept override { return api_version_; }

  void SetApiVersion(std::optional<std::string> api_version) noexcept override {
    api_version_ = std::move(api_version);
  }

  [[nodiscard]] const Request& LastRequest() const { return requests.back(); }

 private:
  void Capture(std::string method, std::string_view path, Headers headers) const {
    requests.push_back({std::move(method), std::string(path), std::move(headers)});
  }

  mutable std::vector<Request> requests;
  std::optional<std::string> api_version_;
};

std::pair<std::unique_ptr<IBucket>, FakeHttpClient*> BuildBucket(std::string api_version) {
  auto client = std::make_unique<FakeHttpClient>(std::move(api_version));
  auto* client_ptr = client.get();
  return {reduct::internal::BuildBucketForTesting("bucket", std::move(client)), client_ptr};
}

void RequireSyncHeader(const Request& request, bool sync) {
  const auto it = request.headers.find(std::string(reduct::internal::kHeaderSyncWrite));
  if (sync) {
    REQUIRE(it != request.headers.end());
    REQUIRE(it->second == "true");
  } else {
    REQUIRE(it == request.headers.end());
  }
}

}  // namespace

TEST_CASE("reduct::IBucket sends sync-write header for writes only", "[sync_write]") {
  SECTION("single record writes") {
    for (const auto sync : {false, true}) {
      auto [bucket, client] = BuildBucket("1.22");
      IBucket::WriteOptions options{.timestamp = IBucket::Time{}};
      options.sync = sync;

      REQUIRE(bucket->Write("entry", options, [](auto record) { record->WriteAll("data"); }) == Error::kOk);
      RequireSyncHeader(client->LastRequest(), sync);
    }

    auto [bucket, client] = BuildBucket("1.22");
    REQUIRE(bucket->Write("entry", IBucket::WriteOptions{.timestamp = IBucket::Time{}},
                          [](auto record) { record->WriteAll("data"); }) == Error::kOk);
    RequireSyncHeader(client->LastRequest(), false);
  }

  SECTION("legacy batch writes") {
    for (const auto sync : {false, true}) {
      auto [bucket, client] = BuildBucket("1.17");
      auto result = bucket->WriteBatch("entry", IBucket::BatchWriteOptions{.sync = sync}, [](auto batch) {
        batch->AddRecord(IBucket::Time{}, "data");
      });

      REQUIRE(result.error == Error::kOk);
      REQUIRE(client->LastRequest().path == "/b/bucket/entry/batch");
      RequireSyncHeader(client->LastRequest(), sync);
    }

    auto [bucket, client] = BuildBucket("1.17");
    REQUIRE(bucket->WriteBatch("entry", [](auto batch) { batch->AddRecord(IBucket::Time{}, "data"); }).error ==
            Error::kOk);
    RequireSyncHeader(client->LastRequest(), false);
  }

  SECTION("multi-entry batch writes") {
    for (const auto sync : {false, true}) {
      auto [bucket, client] = BuildBucket("1.22");
      auto result = bucket->WriteBatch(IBucket::BatchWriteOptions{.sync = sync}, [](auto batch) {
        batch->AddRecord("entry-a", IBucket::Time{}, "data");
        batch->AddRecord("entry-b", IBucket::Time() + std::chrono::microseconds(1), "data");
      });

      REQUIRE(result.error == Error::kOk);
      REQUIRE(client->LastRequest().path == "/io/bucket/write");
      RequireSyncHeader(client->LastRequest(), sync);
    }

    auto [bucket, client] = BuildBucket("1.22");
    REQUIRE(bucket->WriteBatch([](auto batch) { batch->AddRecord("entry", IBucket::Time{}, "data"); }).error ==
            Error::kOk);
    RequireSyncHeader(client->LastRequest(), false);
  }
}

TEST_CASE("reduct::IBucket excludes sync-write header from non-write operations", "[sync_write]") {
  auto [legacy_bucket, legacy_client] = BuildBucket("1.17");
  REQUIRE(legacy_bucket->Update("entry", {.timestamp = IBucket::Time{}, .sync = true}) == Error::kOk);
  RequireSyncHeader(legacy_client->LastRequest(), false);

  REQUIRE(legacy_bucket->UpdateBatch("entry", [](auto batch) {
            batch->AddOnlyLabels(IBucket::Time{}, {{"label", "value"}});
          }).error == Error::kOk);
  RequireSyncHeader(legacy_client->LastRequest(), false);

  REQUIRE(legacy_bucket->RemoveBatch("entry", [](auto batch) { batch->AddRecord(IBucket::Time{}); }).error ==
          Error::kOk);
  RequireSyncHeader(legacy_client->LastRequest(), false);

  auto [bucket, client] = BuildBucket("1.22");
  REQUIRE(bucket->UpdateBatch([](auto batch) {
            batch->AddOnlyLabels("entry", IBucket::Time{}, {{"label", "value"}});
          }).error == Error::kOk);
  RequireSyncHeader(client->LastRequest(), false);

  REQUIRE(bucket->RemoveBatch([](auto batch) { batch->AddRecord("entry", IBucket::Time{}); }).error == Error::kOk);
  RequireSyncHeader(client->LastRequest(), false);

  REQUIRE(bucket->WriteAttachments("entry", {{"metadata", R"({"enabled":true})"}}) == Error::kOk);
  RequireSyncHeader(client->LastRequest(), false);
}
