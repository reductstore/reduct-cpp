// Copyright 2022-2025 ReductSoftware UG

#include "reduct/client.h"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "internal/time_parse.h"
#include "reduct/internal/http_client.h"
#include "reduct/internal/serialisation.h"

namespace reduct {

namespace {

std::string ToIso8601Utc(IClient::Time time) {
  auto tt = std::chrono::system_clock::to_time_t(time);
  std::tm tm = *std::gmtime(&tt);
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
  return oss.str();
}

}  // namespace

using internal::ParseStatus;

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
      auto [default_bucket_settings, def_err] = internal::ParseBucketSettings(data.at("defaults").at("bucket"));
      if (def_err) {
        return {{}, def_err};
      }

      ServerInfo server_info{
          .version = data.at("version"),
          .bucket_count = data.at("bucket_count"),
          .usage = data.at("usage"),
          .uptime = std::chrono::seconds(data.at("uptime")),
          .oldest_record = Time() + std::chrono::microseconds(data.at("oldest_record")),
          .latest_record = Time() + std::chrono::microseconds(data.at("latest_record")),
          .defaults = {.bucket = default_bucket_settings},
      };

      if (data.contains("license") && !data.at("license").is_null()) {
        auto& license = data.at("license");
        server_info.license = ServerInfo::License{
            .licensee = license.at("licensee"),
            .invoice = license.at("invoice"),
            .expiry_date = Time(),
            .plan = license.at("plan"),
            .device_number = license.at("device_number"),
            .disk_quota = license.at("disk_quota"),
            .fingerprint = license.at("fingerprint"),
        };

        server_info.license->expiry_date = parse_iso8601_utc(license.at("expiry_date").get<std::string>());
      }

      return {
          server_info,
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
        bucket_list.push_back({
            .name = bucket.at("name"),
            .entry_count = bucket.at("entry_count"),
            .size = bucket.at("size"),
            .oldest_record = Time() + std::chrono::microseconds(bucket.at("oldest_record")),
            .latest_record = Time() + std::chrono::microseconds(bucket.at("latest_record")),
            .is_provisioned = bucket.value("is_provisioned", false),
            .status = ParseStatus(bucket),
        });
      }
    } catch (const std::exception& e) {
      return {{}, Error{.code = -1, .message = e.what()}};
    }

    return {bucket_list, Error::kOk};
  }

  [[nodiscard]] UPtrResult<IBucket> GetBucket(std::string_view name) const noexcept override {
    auto [_, err] = client_->Head(fmt::format("/b/{}", name));
    if (err) {
      if (err.code == 404) {
        err.message = fmt::format("Bucket '{}' is not found", name);
      }
      return {{}, std::move(err)};
    }

    return {IBucket::Build(url_, name, options_, client_->ApiVersion()), {}};
  }

  [[nodiscard]] UPtrResult<IBucket> CreateBucket(std::string_view name,
                                                 IBucket::Settings settings) const noexcept override {
    auto json_data = internal::BucketSettingToJsonString(settings);
    auto err = client_->Post(fmt::format("/b/{}", name), json_data.is_null() ? "{}" : json_data.dump());
    if (err) {
      return {nullptr, std::move(err)};
    }

    return {IBucket::Build(url_, name, options_, client_->ApiVersion()), {}};
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
        Time created_at = parse_iso8601_utc(token.at("created_at").get<std::string>());

        std::optional<Time> expires_at;
        if (token.contains("expires_at") && !token.at("expires_at").is_null()) {
          expires_at = parse_iso8601_utc(token.at("expires_at").get<std::string>());
        }

        std::optional<Time> last_access;
        if (token.contains("last_access") && !token.at("last_access").is_null()) {
          last_access = parse_iso8601_utc(token.at("last_access").get<std::string>());
        }

        std::optional<uint64_t> ttl;
        if (token.contains("ttl") && !token.at("ttl").is_null()) {
          ttl = token.at("ttl").get<uint64_t>();
        }

        token_list.push_back(Token{
            .name = token.at("name"),
            .created_at = created_at,
            .is_provisioned = token.value("is_provisioned", false),
            .expires_at = expires_at,
            .ttl = ttl,
            .last_access = last_access,
            .ip_allowlist = token.value("ip_allowlist", std::vector<std::string>{}),
            .is_expired = token.value("is_expired", false),
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

  Result<std::string> CreateToken(std::string_view name, TokenCreateRequest request) const noexcept override {
    nlohmann::json json_data;

    auto api_version = client_->ApiVersion();
    if (!api_version.has_value()) {
      auto [_, info_err] = client_->Get("/info");
      if (info_err) {
        return Result<std::string>{{}, std::move(info_err)};
      }
      api_version = client_->ApiVersion();
    }

    auto supports_v2 = api_version && internal::IsCompatible("1.19", *api_version);
    if (supports_v2) {
      json_data["permissions"] = {
          {"full_access", request.permissions.full_access},
          {"read", std::move(request.permissions.read)},
          {"write", std::move(request.permissions.write)},
      };

      if (request.expires_at.has_value()) {
        json_data["expires_at"] = ToIso8601Utc(*request.expires_at);
      }

      if (request.ttl.has_value()) {
        json_data["ttl"] = *request.ttl;
      }

      json_data["ip_allowlist"] = std::move(request.ip_allowlist);
    } else {
      json_data["full_access"] = request.permissions.full_access;
      json_data["read"] = std::move(request.permissions.read);
      json_data["write"] = std::move(request.permissions.write);
    }

    auto [body, err] = client_->PostWithResponse(fmt::format("/tokens/{}", name), json_data.dump());
    if (err && supports_v2 && err.code == 422) {
      nlohmann::json legacy_json_data;
      legacy_json_data["full_access"] = request.permissions.full_access;
      legacy_json_data["read"] = request.permissions.read;
      legacy_json_data["write"] = request.permissions.write;
      auto legacy_ret = client_->PostWithResponse(fmt::format("/tokens/{}", name), legacy_json_data.dump());
      body = std::move(legacy_ret.result);
      err = std::move(legacy_ret.error);
    }

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

  Result<std::string> RotateToken(std::string_view name) const noexcept override {
    auto [body, err] = client_->PostWithResponse(fmt::format("/tokens/{}/rotate", name), "{}");
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

  Result<std::vector<ReplicationInfo>> GetReplicationList() const noexcept override {
    auto [body, err] = client_->Get("/replications");
    if (err) {
      return {{}, std::move(err)};
    }

    try {
      nlohmann::json data = nlohmann::json::parse(body);
      return internal::ParseReplicationList(data);
    } catch (const std::exception& e) {
      return {{}, Error{.code = -1, .message = e.what()}};
    }
  }

  Result<FullReplicationInfo> GetReplication(std::string_view name) const noexcept override {
    auto [body, err] = client_->Get(fmt::format("/replications/{}", name));
    if (err) {
      return {{}, std::move(err)};
    }

    try {
      nlohmann::json data = nlohmann::json::parse(body);
      return internal::ParseFullReplicationInfo(data);
    } catch (const std::exception& e) {
      return {{}, Error{.code = -1, .message = e.what()}};
    }
  }

  Error CreateReplication(std::string_view name, ReplicationSettings settings) const noexcept override {
    auto [json_data, json_err] = internal::ReplicationSettingsToJsonString(std::move(settings));
    if (json_err) {
      return json_err;
    }

    return client_->Post(fmt::format("/replications/{}", name), json_data.dump());
  }

  Error UpdateReplication(std::string_view name, ReplicationSettings settings) const noexcept override {
    auto [json_data, json_err] = internal::ReplicationSettingsToJsonString(std::move(settings));
    if (json_err) {
      return json_err;
    }

    return client_->Put(fmt::format("/replications/{}", name), json_data.dump());
  }

  Error SetReplicationMode(std::string_view name, ReplicationMode mode) const noexcept override {
    try {
      nlohmann::json payload = {{"mode", internal::ReplicationModeToString(mode)}};
      auto patch_result = client_->Patch(fmt::format("/replications/{}/mode", name), payload.dump(),
                                         {{"Content-Type", "application/json"}});
      return patch_result.error;
    } catch (const std::exception& ex) {
      return Error{.code = -1, .message = ex.what()};
    }
  }

  Error RemoveReplication(std::string_view name) const noexcept override {
    return client_->Delete(fmt::format("/replications/{}", name));
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
