// Copyright 2022 Alexey Timin

#include "reduct/internal/http_client.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

namespace reduct::internal {

class HttpClient : public IHttpClient {
 public:
  explicit HttpClient(std::string_view url) : client_(std::make_unique<httplib::Client>(std::string(url))) {}

  Result<std::string> Get(std::string_view path) const noexcept override {
    auto res = client_->Get(path.data());
    if (auto err = CheckRequest(res)) {
      return {{}, std::move(err)};
    }

    return {std::move(res->body), Error::kOk};
  }

  Error Head(std::string_view path) const noexcept override {
    auto res = client_->Head(path.data());
    return CheckRequest(res);
  }

  Error Post(std::string_view path, std::string_view body, std::string_view mime) const noexcept override {
    auto res = client_->Post(path.data(), body.data(), mime.data());
    return CheckRequest(res);
  }

  Error Put(std::string_view path, std::string_view body, std::string_view mime) const noexcept override {
    auto res = client_->Put(path.data(), std::string(body), mime.data());
    return CheckRequest(res);
  }

  Error Delete(std::string_view path) const noexcept override {
    auto res = client_->Delete(path.data());
    return CheckRequest(res);
  }

 private:
  static Error CheckRequest(const httplib::Result& res) {
    if (res.error() != httplib::Error::Success) {
      return Error{.code = -1, .message = httplib::to_string(res.error())};
    }

    if (res->status != 200) {
      return ParseDetail(res);
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
};

std::unique_ptr<IHttpClient> IHttpClient::Build(std::string_view url) { return std::make_unique<HttpClient>(url); }
}  // namespace reduct::internal
