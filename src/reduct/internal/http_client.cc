// Copyright 2022 Alexey Timin

#include "reduct/internal/http_client.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include "sha256.h"

namespace reduct::internal {

class HttpClient : public IHttpClient {
 public:
  explicit HttpClient(std::string_view url, std::string_view api_token)
      : client_(std::make_unique<httplib::Client>(std::string(url))) {
    std::vector<unsigned char> hash(picosha2::k_digest_size);
    picosha2::hash256(api_token.begin(), api_token.end(), hash.begin(), hash.end());
    api_token_ = picosha2::bytes_to_hex_string(hash.begin(), hash.end());
  }

  Result<std::string> Get(std::string_view path) const noexcept override {
    auto res = AuthWrapper([this, path] { return client_->Get(path.data()); });
    if (auto err = CheckRequest(res)) {
      return {{}, std::move(err)};
    }

    return {std::move(res->body), Error::kOk};
  }

  Error Head(std::string_view path) const noexcept override {
    auto res = AuthWrapper([this, path] { return client_->Head(path.data()); });
    return CheckRequest(res);
  }

  Error Post(std::string_view path, std::string_view body, std::string_view mime) const noexcept override {
    auto res = AuthWrapper([this, path, body, mime] { return client_->Post(path.data(), body.data(), mime.data()); });
    return CheckRequest(res);
  }

  Error Put(std::string_view path, std::string_view body, std::string_view mime) const noexcept override {
    auto res =
        AuthWrapper([this, path, body, mime] { return client_->Put(path.data(), std::string(body), mime.data()); });
    return CheckRequest(res);
  }

  Error Delete(std::string_view path) const noexcept override {
    auto res = AuthWrapper([this, path] { return client_->Delete(path.data()); });
    return CheckRequest(res);
  }

 private:
  httplib::Result AuthWrapper(const std::function<httplib::Result()>& req) const noexcept {
    auto res = req();
    if (res.error() == httplib::Error::Success && res->status == 401) {
      client_->set_bearer_token_auth(api_token_.data());
      res = client_->Post("/auth/refresh");
      if (res.error() == httplib::Error::Success && res->status == 200) {
        try {
          nlohmann::json data;
          data = nlohmann::json::parse(res->body);
          access_token_ = data.at("access_token");
          client_->set_bearer_token_auth(access_token_.data());
          res = req();  // repeat request with the new access token
        } catch (const std::exception& e) {
          // TODO(Alexey Timin): Needs rethinking error handling
        }
      }
    }
    return res;
  }

  static Error CheckRequest(const httplib::Result& res) {
    if (res.error() != httplib::Error::Success) {
      return Error{.code = -1, .message = httplib::to_string(res.error())};
    }

    if (res->status != 200) {
      if (res->body.empty()) {
        return {.code = res->status, .message = "HTTP Error"};
      } else {
        return ParseDetail(res);
      }
    }

    return Error::kOk;
  }

  static Error ParseDetail(const httplib::Result& res) {
    try {
      nlohmann::json data;
      data = nlohmann::json::parse(res->body);
      return Error{.code = res->status, .message = data["detail"]};
    } catch (const std::exception& e) {
      return Error{.code = -1, .message = e.what()};
    }
  }

  std::unique_ptr<httplib::Client> client_;
  std::string api_token_;
  mutable std::string access_token_;
};

std::unique_ptr<IHttpClient> IHttpClient::Build(std::string_view url, std::string_view api_token) {
  return std::make_unique<HttpClient>(url, api_token);
}
}  // namespace reduct::internal
