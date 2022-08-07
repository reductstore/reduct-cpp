// Copyright 2022 Alexey Timin

#include "reduct/internal/http_client.h"
#undef CPPHTTPLIB_BROTLI_SUPPORT

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <algorithm>

namespace reduct::internal {

using httplib::DataSink;

class HttpClient : public IHttpClient {
 public:
  explicit HttpClient(std::string_view url, const HttpOptions& options)
      : client_(std::make_unique<httplib::Client>(std::string(url))), api_token_(options.api_token) {
    client_->enable_server_certificate_verification(options.ssl_verification);
  }

  Result<std::string> Get(std::string_view path) const noexcept override {
    auto res = AuthWrapper([this, path] { return client_->Get(path.data()); });
    if (auto err = CheckRequest(res)) {
      return {{}, std::move(err)};
    }

    return {std::move(res->body), Error::kOk};
  }

  Error Get(std::string_view path, ResponseCallback resp_callback, ReadCallback read_callback) const noexcept override {
    Error err = Error::kOk;
    std::string err_body;
    auto res = AuthWrapper([this, &err, &err_body, path, resp_cb = resp_callback, read_cb = std::move(read_callback)] {
      return client_->Get(
          path.data(),
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

            resp_cb(std::move(headers));
            return true;
          },
          [&](const char* data, size_t size) {
            if (err) {
              err_body.append(std::string_view(data, size));
              return true;
            }

            return read_cb(std::string_view(data, size));
          });
    });
    return CheckRequest(res, err_body);
  }

  Error Head(std::string_view path) const noexcept override {
    auto res = AuthWrapper([this, path] { return client_->Head(path.data()); });
    return CheckRequest(res);
  }

  Error Post(std::string_view path, std::string_view body, std::string_view mime) const noexcept override {
    auto res = AuthWrapper([this, path, body, mime] { return client_->Post(path.data(), body.data(), mime.data()); });
    return CheckRequest(res);
  }

  Error Post(std::string_view path, std::string_view mime, size_t content_length,
             WriteCallback callback) const noexcept override {
    auto res = AuthWrapper([this, path, mime, content_length, clb = std::move(callback)] {
      return client_->Post(
          path.data(), content_length,
          [&](size_t offset, size_t size, DataSink& sink) {
            auto [ok, data] = clb(offset, size);
            sink.write(data.data(), data.size());
            return ok;
          },
          mime.data());
    });
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

  std::unique_ptr<httplib::Client> client_;
  std::string api_token_;
  mutable std::string access_token_;
};

std::unique_ptr<IHttpClient> IHttpClient::Build(std::string_view url, const HttpOptions& options) {
  return std::make_unique<HttpClient>(url, options);
}
}  // namespace reduct::internal
