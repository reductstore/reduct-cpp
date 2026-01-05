// Copyright 2022 ReductSoftware UG
// Copyright 2024-2025 ReductSoftware UG

#ifndef REDUCTCPP_HTTP_OPTIONS_H
#define REDUCTCPP_HTTP_OPTIONS_H

#include <string>
#include <optional>
#include <chrono>

namespace reduct {
/**
 * Client options
 */
struct HttpOptions {
  std::string api_token;  // API token, if empty anonymous access
  bool ssl_verification;  // check ssl certificate if it is true
  std::optional<std::chrono::milliseconds> connection_timeout;
  std::optional<std::chrono::milliseconds> request_timeout;

  auto operator<=>(const HttpOptions&) const = default;
};

constexpr std::string_view kApiPrefix = "/api/v1";

}  // namespace reduct
#endif  // REDUCTCPP_HTTP_OPTIONS_H
