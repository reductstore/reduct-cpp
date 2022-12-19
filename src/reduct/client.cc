// Copyright 2022 Alexey Timin

#include "reduct/client.h"

#include <date/date.h>
#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include <iostream>

#include "reduct/internal/http_client.h"
#include "reduct/internal/serialisation.h"

namespace reduct {

/**
 * Hidden implement of IClient.
 */
class Client : public IClient {
 public:
  explicit Client(std::string_view url, HttpOptions options) : url_(url), options_(std::move(options)) {
    client_ = internal::IHttpClient::Build(url_, options_);
  }

  [[nodiscard]] Result<ServerInfo> GetInfo() const noexcept override {
    auto [body, err] = client_->Get("/info");
    if (err) {
      return {{}, std::move(err)};
    }

    try {
      nlohmann::json data = nlohmann::json::parse(body);
      auto as_ul = [&data](std::string_view key) { return std::stoul(data.at(key.data()).get<std::string>()); };

      auto [default_bucket_settings, def_err] = internal::ParseBucketSettings(data.at("defaults").at("bucket"));
      if (def_err) {
        return {{}, def_err};
      }
      return {
          ServerInfo{.version = data.at("version"),
                     .bucket_count = as_ul("bucket_count"),
                     .usage = as_ul("usage"),
                     .uptime = std::chrono::seconds(as_ul("uptime")),
                     .oldest_record = Time() + std::chrono::microseconds(as_ul("oldest_record")),
                     .latest_record = Time() + std::chrono::microseconds(as_ul("latest_record")),
                     .defaults = {.bucket = default_bucket_settings}},
          Error::kOk,
      };
    } catch (const std::exception& e) {
      return {{}, Error{.code = -1, .message = e.what()}};
    }
  }

  Result<std::vector<IBucket::BucketInfo>> GetBucketList() const noexcept override {
    auto [body, err] = client_->Get("/list");
    if (err) {
      return {{}, std::move(err)};
    }

    std::vector<IBucket::BucketInfo> bucket_list;
    try {
      nlohmann::json data = nlohmann::json::parse(body);

      auto json_buckets = data.at("buckets");
      bucket_list.reserve(json_buckets.size());
      for (const auto& bucket : json_buckets) {
        auto as_ul = [&bucket](std::string_view key) { return std::stoul(bucket.at(key.data()).get<std::string>()); };
        bucket_list.push_back({
            .name = bucket.at("name"),
            .entry_count = as_ul("entry_count"),
            .size = as_ul("size"),
            .oldest_record = Time() + std::chrono::microseconds(as_ul("oldest_record")),
            .latest_record = Time() + std::chrono::microseconds(as_ul("latest_record")),
        });
      }
    } catch (const std::exception& e) {
      return {{}, Error{.code = -1, .message = e.what()}};
    }

    return {bucket_list, Error::kOk};
  }

  [[nodiscard]] UPtrResult<IBucket> GetBucket(std::string_view name) const noexcept override {
    auto err = client_->Head(fmt::format("/b/{}", name));
    if (err) {
      if (err.code == 404) {
        err.message = fmt::format("Bucket '{}' is not found", name);
      }
      return {{}, std::move(err)};
    }

    return {IBucket::Build(url_, name, options_), {}};
  }

  [[nodiscard]] UPtrResult<IBucket> CreateBucket(std::string_view name,
                                                 IBucket::Settings settings) const noexcept override {
    auto json_data = internal::BucketSettingToJsonString(settings);
    auto err = client_->Post(fmt::format("/b/{}", name), json_data.is_null() ? "{}" : json_data.dump());
    if (err) {
      return {nullptr, std::move(err)};
    }

    return {IBucket::Build(url_, name, options_), {}};
  }

  UPtrResult<IBucket> GetOrCreateBucket(std::string_view name, IBucket::Settings settings) const noexcept override {
    auto ret = GetBucket(name);
    if (ret.error.code == 404) {
      return CreateBucket(name, settings);
    }

    return ret;
  }
  Result<std::vector<Token>> GetTokenList() const noexcept override {
    auto [body, err] = client_->Get("/tokens");
    if (err) {
      return {{}, std::move(err)};
    }

    std::vector<Token> token_list;
    try {
      nlohmann::json data = nlohmann::json::parse(body);

      auto json_tokens = data.at("tokens");
      token_list.reserve(json_tokens.size());
      for (const auto& token : json_tokens) {
        Time created_at;
        std::istringstream(token.at("created_at").get<std::string>()) >> date::parse("%FT%TZ", created_at);

        token_list.push_back(Token{
            .name = token.at("name"),
            .created_at = created_at,
        });
      }
    } catch (const std::exception& e) {
      return {{}, Error{.code = -1, .message = e.what()}};
    }

    return {token_list, Error::kOk};
  }

  Result<FullTokenInfo> GetToken(std::string_view name) const noexcept override {
    auto [body, err] = client_->Get(fmt::format("/tokens/{}", name));
    if (err) {
      return {{}, std::move(err)};
    }

    try {
      nlohmann::json data = nlohmann::json::parse(body);
      return internal::ParseTokenInfo(data);
    } catch (const std::exception& e) {
      return {{}, Error{.code = -1, .message = e.what()}};
    }
  }

  Result<std::string> CreateToken(std::string_view name, Permissions permissions) const noexcept override {
    nlohmann::json json_data;
    json_data["full_access"] = permissions.full_access;
    json_data["read"] = std::move(permissions.read);
    json_data["write"] = std::move(permissions.write);

    auto [body, err] = client_->PostWithResponse(fmt::format("/tokens/{}", name), json_data.dump());
    if (err) {
      return Result<std::string>{{}, std::move(err)};
    }

    try {
      nlohmann::json data = nlohmann::json::parse(body);
      return {data.at("value").get<std::string>(), Error::kOk};
    } catch (const std::exception& e) {
      return {{}, Error{.code = -1, .message = e.what()}};
    }
  }

  Error RemoveToken(std::string_view name) const noexcept override {
    return client_->Delete(fmt::format("/tokens/{}", name));
  }

  Result<FullTokenInfo> Me() const noexcept override {
    auto [body, err] = client_->Get("/me");
    if (err) {
      return {{}, std::move(err)};
    }

    try {
      nlohmann::json data = nlohmann::json::parse(body);
      return internal::ParseTokenInfo(data);
    } catch (const std::exception& e) {
      return {{}, Error{.code = -1, .message = e.what()}};
    }
  }

 private:
  HttpOptions options_;
  std::unique_ptr<internal::IHttpClient> client_;
  std::string url_;
};

std::unique_ptr<IClient> IClient::Build(std::string_view url, HttpOptions options) noexcept {
  return std::make_unique<Client>(url, std::move(options));
}

}  // namespace reduct
