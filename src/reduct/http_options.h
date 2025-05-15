// Copyright 2022 Alexey Timin

#ifndef REDUCTCPP_HTTP_OPTIONS_H
#define REDUCTCPP_HTTP_OPTIONS_H

#include <string>

namespace reduct {
/**
 * Client options
 */
struct HttpOptions {
  std::string api_token;  // API token, if empty anonymous access
  bool ssl_verification;  // check ssl certificate if it is true

  auto operator<=>(const HttpOptions&) const = default;
};

constexpr std::string_view kApiPrefix = "/api/v1";

}  // namespace reduct
#endif  // REDUCTCPP_HTTP_OPTIONS_H
