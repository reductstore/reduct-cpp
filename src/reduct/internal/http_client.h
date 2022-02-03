// Copyright 2022 Alexey Timin

#ifndef REDUCT_CPP_HTTP_CLIENT_H
#define REDUCT_CPP_HTTP_CLIENT_H

#include <memory>
#include <string_view>

#include "reduct/result.h"

namespace reduct::internal {

/**
 * Wrapper for HTTP client
 */
class IHttpClient {
 public:
  virtual Result<std::string> Get(std::string_view path) const noexcept = 0;
  virtual Error Post(std::string_view path, std::string_view body) const noexcept = 0;
  virtual Error Put(std::string_view path, std::string_view body) const noexcept = 0;
  virtual Error Delete(std::string_view path) const noexcept = 0;

  static std::unique_ptr<IHttpClient> Build(std::string_view url);
};

}  // namespace reduct::internal
#endif  // REDUCT_CPP_HTTP_CLIENT_H
