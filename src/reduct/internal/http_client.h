// Copyright 2022-2023 ReductSoftware UG

#ifndef REDUCT_CPP_HTTP_CLIENT_H
#define REDUCT_CPP_HTTP_CLIENT_H

#include <functional>
#include <memory>
#include <optional>
#include <string_view>

#include "reduct/http_options.h"
#include "reduct/result.h"

namespace reduct::internal {

/**
 * Wrapper for HTTP client
 */
class IHttpClient {
 public:
  virtual Result<std::string> Get(std::string_view path) const noexcept = 0;
  using WriteCallback = std::function<std::pair<bool, std::string>(size_t offset, size_t size)>;

  using ReadCallback = std::function<bool(std::string_view)>;
  using Headers = std::unordered_map<std::string, std::string>;
  using ResponseCallback = std::function<void(Headers &&)>;

  virtual Error Get(std::string_view path, ResponseCallback, ReadCallback) const noexcept = 0;

  virtual Error Get(std::string_view path, Headers headers, ResponseCallback, ReadCallback) const noexcept = 0;

  virtual Result<Headers> Head(std::string_view path) const noexcept = 0;

  virtual Result<Headers> Head(std::string_view path, Headers headers) const noexcept = 0;

  virtual Error Post(std::string_view path, std::string_view body,
                     std::string_view mime = "application/json") const noexcept = 0;

  virtual Result<std::string> PostWithResponse(std::string_view path, std::string_view body,
                                               std::string_view mime = "application/json") const noexcept = 0;

  virtual Result<std::tuple<std::string, Headers>> Post(std::string_view path, std::string_view mime,
                                                        size_t content_length, Headers headers,
                                                        WriteCallback) const noexcept = 0;

  virtual Error Put(std::string_view path, std::string_view body,
                    std::string_view mime = "application/json") const noexcept = 0;

  virtual Result<std::tuple<std::string, Headers>> Patch(std::string_view path, std::string_view body,
                                                         Headers headers) const noexcept = 0;

  virtual Result<std::tuple<std::string, Headers>> Delete(std::string_view path,
                                                          Headers headers = {}) const noexcept = 0;

  [[nodiscard]] virtual std::optional<std::string> ApiVersion() const noexcept = 0;

  static std::unique_ptr<IHttpClient> Build(std::string_view url, const HttpOptions &options);
};

bool IsCompatible(std::string_view min, std::string_view version);

}  // namespace reduct::internal
#endif  // REDUCT_CPP_HTTP_CLIENT_H
