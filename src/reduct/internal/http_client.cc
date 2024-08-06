// Copyright 2022-2024 Alexey Timin

#include "reduct/internal/http_client.h"
#undef CPPHTTPLIB_BROTLI_SUPPORT

#include <fmt/format.h>
#include <httplib.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <string>

namespace reduct::internal {

using httplib::DataSink;

constexpr size_t kMaxChunkSize = 512'000;

Result<IHttpClient::Headers> NormalizeHeaders(httplib::Result res) {
  IHttpClient::Headers response_headers;
  for (auto& [k, v] : res->headers) {
    std::string lowcase_header = k;
    std::transform(k.begin(), k.end(), lowcase_header.begin(), [](auto ch) { return tolower(ch); });
    response_headers[lowcase_header] = v;
  }

  return {std::move(response_headers), Error::kOk};
}

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
    return CheckRequest(res);
  }

  Result<Headers> Head(std::string_view path) const noexcept override {
    auto res = client_->Head(AddApiPrefix(path).data());
    auto err = CheckRequest(res);
    if (err) {
      return {{}, std::move(err)};
    }

    Headers headers;
    for (auto& [k, v] : res->headers) {
      std::string lowcase_header = k;
      std::transform(k.begin(), k.end(), lowcase_header.begin(), [](auto ch) { return std::tolower(ch); });
      headers[lowcase_header] = v;
    }

    return {std::move(headers), Error::kOk};
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

  Result<Headers> Post(std::string_view path, std::string_view mime, size_t content_length, Headers headers,
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

    if (auto err = CheckRequest(res)) {
      return {{}, std::move(err)};
    }

    return NormalizeHeaders(std::move(res));
  }

  Error Put(std::string_view path, std::string_view body, std::string_view mime) const noexcept override {
    auto res = client_->Put(AddApiPrefix(path).data(), std::string(body), mime.data());
    return CheckRequest(res);
  }

  Result<Headers> Patch(std::string_view path, std::string_view body, Headers headers) const noexcept override {
    httplib::Headers httplib_headers;
    for (auto& [k, v] : headers) {
      httplib_headers.emplace(k, v);
    }
    auto res = client_->Patch(AddApiPrefix(path).data(), httplib_headers, std::string(body), "");
    if (auto err = CheckRequest(res)) {
      return {{}, std::move(err)};
    }

    return NormalizeHeaders(std::move(res));
  }

  Error Delete(std::string_view path) const noexcept override {
    auto res = client_->Delete(AddApiPrefix(path).data());
    return CheckRequest(res);
  }

  std::string_view api_version() const noexcept override { return api_version_; }

 private:
  Error CheckRequest(const httplib::Result& res) const noexcept {
    if (res.error() != httplib::Error::Success) {
      return Error{.code = -1, .message = httplib::to_string(res.error())};
    }

    auto status = res->status;
    if (status != 200) {
      auto msg = res->headers.find("x-reduct-error");
      if (msg != res->headers.end()) {
        return Error{.code = status, .message = msg->second};
      }

      return Error{.code = status, .message = "Unknown error"};
    }

    if (auto api_version = res->headers.find("x-reduct-api"); api_version != res->headers.end()) {
      api_version_ = api_version->second;
    }

    return Error::kOk;
  }

  static std::string AddApiPrefix(std::string_view path) { return fmt::format("{}{}", kApiPrefix, path); }

  std::unique_ptr<httplib::Client> client_;
  std::string api_token_;
  mutable std::string access_token_;
  mutable std::string api_version_;
};

std::unique_ptr<IHttpClient> IHttpClient::Build(std::string_view url, const HttpOptions& options) {
  return std::make_unique<HttpClient>(url, options);
}

bool IsCompatible(std::string_view min, std::string_view version) {
  if (version.empty()) {
    return false;
  }

  auto parse_version = [](std::string version) {
    std::stringstream ss(version);
    std::string item;
    std::vector<int> result;
    while (std::getline(ss, item, '.')) {
      result.push_back(std::stoi(item));
    }
    return result;
  };

  auto min_version = parse_version(std::string(min));
  auto current_version = parse_version(std::string(version));

  if (min_version.size() != current_version.size()) {
    return false;
  }

  if (min_version.size() != 2) {
    return false;
  }

  return min_version[0] == current_version[0] && min_version[1] <= current_version[1];
}

}  // namespace reduct::internal
