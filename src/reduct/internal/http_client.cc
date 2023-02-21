// Copyright 2022 Alexey Timin

#include "reduct/internal/http_client.h"
#undef CPPHTTPLIB_BROTLI_SUPPORT

#include <fmt/format.h>
#include <httplib.h>
#include <nlohmann/json.hpp>

#include <algorithm>

namespace reduct::internal {

using httplib::DataSink;

constexpr size_t kMaxChunkSize = 1'000'000;

class HttpClient : public IHttpClient {
 public:
  explicit HttpClient(std::string_view url, const HttpOptions& options)
      : client_(std::make_unique<httplib::Client>(std::string(url))), api_token_(options.api_token) {
    client_->enable_server_certificate_verification(options.ssl_verification);

    if (!options.api_token.empty()) {
      client_->set_bearer_token_auth(options.api_token.data());
    }
  }

  Result<std::string> Get(std::string_view path) const noexcept override {
    auto res = client_->Get(AddApiPrefix(path).data());
    if (auto err = CheckRequest(res)) {
      return {{}, std::move(err)};
    }

    return {std::move(res->body), Error::kOk};
  }

  Error Get(std::string_view path, ResponseCallback resp_callback, ReadCallback read_callback) const noexcept override {
    Error err = Error::kOk;
    std::string err_body;
    auto res = client_->Get(
        AddApiPrefix(path).data(),
        [&](const auto& response) {
          if (response.status != 200) {
            err.code = response.status;
          }

          Headers headers;
          for (auto& [k, v] : response.headers) {
            std::string lowcase_header = k;
            std::transform(k.begin(), k.end(), lowcase_header.begin(), [](auto ch) { return std::tolower(ch); });
            headers[lowcase_header] = v;
          }

          if (!err) {
            resp_callback(std::move(headers));
          }
          return true;
        },
        [&](const char* data, size_t size) {
          if (err) {
            err_body.append(std::string_view(data, size));
            return true;
          }

          return read_callback(std::string_view(data, size));
        });
    return CheckRequest(res, err_body);
  }

  Error Head(std::string_view path) const noexcept override {
    auto res = client_->Head(AddApiPrefix(path).data());
    return CheckRequest(res);
  }

  Error Post(std::string_view path, std::string_view body, std::string_view mime) const noexcept override {
    auto [_, error] = PostWithResponse(path, body.data(), mime.data());
    return error;
  }

  Result<std::string> PostWithResponse(std::string_view path, std::string_view body,
                                       std::string_view mime) const noexcept override {
    auto res = client_->Post(AddApiPrefix(path).data(), body.data(), mime.data());
    if (auto err = CheckRequest(res)) {
      return {{}, std::move(err)};
    }

    return {std::move(res->body), Error::kOk};
  }

  Error Post(std::string_view path, std::string_view mime, size_t content_length, Headers headers,
             WriteCallback callback) const noexcept override {
    httplib::Headers httplib_headers;
    for (auto& [k, v] : headers) {
      httplib_headers.emplace(k, v);
    }
    auto res = client_->Post(
        AddApiPrefix(path).data(), httplib_headers, content_length,
        [&](size_t offset, size_t size, DataSink& sink) {
          size = std::min<size_t>(size, kMaxChunkSize);
          auto [ok, data] = callback(offset, size);
          sink.write(data.data(), size);
          return ok;
        },
        mime.data());
    return CheckRequest(res);
  }

  Error Put(std::string_view path, std::string_view body, std::string_view mime) const noexcept override {
    auto res = client_->Put(AddApiPrefix(path).data(), std::string(body), mime.data());
    return CheckRequest(res);
  }

  Error Delete(std::string_view path) const noexcept override {
    auto res = client_->Delete(AddApiPrefix(path).data());
    return CheckRequest(res);
  }

 private:
  static Error CheckRequest(const httplib::Result& res, std::string_view body = {}) {
    auto parse_detail = [](int status, std::string_view body) -> Error {
      try {
        nlohmann::json data;
        data = nlohmann::json::parse(body);
        return Error{.code = status, .message = data["detail"]};
      } catch (const std::exception& e) {
        return Error{.code = -1, .message = e.what()};
      }
    };

    if (res.error() != httplib::Error::Success) {
      return Error{.code = -1, .message = httplib::to_string(res.error())};
    }

    auto status = res->status;
    if (status != 200) {
      if (!body.empty()) {
        return parse_detail(status, body);
      }
      if (res->body.empty()) {
        return {.code = res->status, .message = "HTTP Error"};
      }

      return parse_detail(status, res->body);
    }

    return Error::kOk;
  }

  static std::string AddApiPrefix(std::string_view path) { return fmt::format("{}{}", kApiPrefix, path); }

  std::unique_ptr<httplib::Client> client_;
  std::string api_token_;
  mutable std::string access_token_;
};

std::unique_ptr<IHttpClient> IHttpClient::Build(std::string_view url, const HttpOptions& options) {
  return std::make_unique<HttpClient>(url, options);
}
}  // namespace reduct::internal
