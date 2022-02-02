// Copyright 2022 Alexey Timin

#include "reduct/client.h"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include "reduct/internal/http_client.h"

namespace reduct {

class Bucket : public IBucket {
 public:
  Bucket(std::string_view url, std::string_view name) : name_(name) { client_ = internal::IHttpClient::Build(url); }

  Result<Settings> GetSettings() const noexcept override {
    auto [body, err] = client_->Get(fmt::format("/b/{}", name_));
    if (err) {
      return {{}, std::move(err)};
    }
    return Settings::Parse(body);
  }

  Error UpdateSettings(const Settings& settings) const noexcept override {
    auto [current_setting, get_err] = GetSettings();
    if (get_err) {
      return get_err;
    }

    // TODO(Alexey Timin): Make PUT request parameters optional to avoid this
    if (settings.max_block_size) current_setting.max_block_size = settings.max_block_size;
    if (settings.quota_type) current_setting.quota_type = settings.quota_type;
    if (settings.quota_size) current_setting.quota_size = settings.quota_size;

    return client_->Put(fmt::format("/b/{}", name_), current_setting.ToJsonString());
  }

  ReadResult Read(std::string_view entry_name, Time ts) const override { return ReadResult(); }
  Error Write(std::string_view entry_name, std::string_view data, Time ts) const override { return Error(); }
  ListResult List(std::string_view entry_name, Time start, Time stop) const override { return ListResult(); }
  Error Remove() const override { return Error(); }

 private:
  std::unique_ptr<internal::IHttpClient> client_;
  std::string name_;
};

/**
 * Hidden implement of IClient.
 */
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

  [[nodiscard]] UPtrResult<IBucket> GetBucket(std::string_view name) const noexcept override {
    auto [body, err] = client_->Get(fmt::format("/b/{}", name));
    if (err) {
      return {{}, std::move(err)};
    }

    return {std::make_unique<Bucket>(url_, name), {}};
  }

  [[nodiscard]] UPtrResult<IBucket> CreateBucket(std::string_view name,
                                                 IBucket::Settings settings) const noexcept override {
    auto err = client_->Post(fmt::format("/b/{}", name), settings.ToJsonString());
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
