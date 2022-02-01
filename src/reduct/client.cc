// Copyright 2022 Alexey Timin

#include "reduct/client.h"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include "reduct/internal/http_client.h"

namespace reduct {

class Bucket : public IBucket {
 public:
  Bucket(std::string_view url, std::string_view name) {}
  ReadResult Read(std::string_view entry_name, Time ts) const override { return ReadResult(); }
  Error Write(std::string_view entry_name, std::string_view data, Time ts) const override { return Error(); }
  ListResult List(std::string_view entry_name, Time start, Time stop) const override { return ListResult(); }
  Error Remove() const override { return Error(); }
};

class Client : public IClient {
 public:
  explicit Client(std::string_view url) : url_(url) { client_ = internal::IHttpClient::Build(url); }

  [[nodiscard]] Result<ServerInfo> GetInfo() const noexcept override {
    auto [body, err] = client_->Get("/info");
    if (err) {
      return {{}, std::move(err)};
    }

    try {
      nlohmann::json data;
      data = nlohmann::json::parse(body);
      return {
          ServerInfo{
              .version = data.at("version"),
              .bucket_count = data.at("bucket_count"),
          },
          Error::kOk,
      };
    } catch (const std::exception& e) {
      return {{}, Error{.code = -1, .message = e.what()}};
    }
  }

  [[nodiscard]] UPtrResult<IBucket> GetBucket(std::string_view name) const noexcept override { return {}; }
  [[nodiscard]] UPtrResult<IBucket> CreateBucket(std::string_view name,
                                                 IBucket::Settings settings) const noexcept override {
    nlohmann::json data;
    if (settings.max_block_size) {
      data["max_block_size"] = *settings.max_block_size;
    }

    if (settings.quota_type) {
      switch (*settings.quota_type) {
        case IBucket::QuotaType::kNone:
          data["quota_type"] = "NONE";
          break;
        case IBucket::QuotaType::kFifo:
          data["quota_type"] = "FIFO";
          break;
      }
    }

    if (settings.quota_size) {
      data["quota_size"] = *settings.quota_type;
    }

    auto err = client_->Post(fmt::format("/b/{}", name), data.dump());
    if (err) {
      return {nullptr, std::move(err)};
    }

    return {std::make_unique<Bucket>(url_, name), {}};
  }

 private:
  std::unique_ptr<internal::IHttpClient> client_;
  std::string url_;
};

std::unique_ptr<IClient> IClient::Build(std::string_view url) { return std::make_unique<Client>(url); }
}  // namespace reduct
