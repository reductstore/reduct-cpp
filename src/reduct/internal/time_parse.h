// Copyright 2025 ReductSoftware UG

#ifndef REDUCTCPP_TIME_PARSE_H

#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

namespace reduct {

// Parse ISO8601 like "2024-10-11T13:45:30Z" or "2024-10-11T13:45:30"
inline std::chrono::system_clock::time_point parse_iso8601_utc(const std::string& iso_str) {
  // Must end with 'Z' or 'z'
  if (iso_str.empty() || (iso_str.back() != 'Z' && iso_str.back() != 'z')) {
    throw std::runtime_error("Invalid timestamp (missing 'Z'): " + iso_str);
  }

  // Remove trailing 'Z' and fractional part (if present)
  std::string clean = iso_str.substr(0, iso_str.size() - 1);
  std::size_t dot_pos = clean.find('.');
  if (dot_pos != std::string::npos) {
    clean = clean.substr(0, dot_pos);  // strip everything after '.'
  }

  std::tm tm = {};
  std::istringstream ss(clean);

  ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");

#if defined(_WIN32)
  std::time_t time = _mkgmtime(&tm);  // Windows
#else
  std::time_t time = timegm(&tm);  // POSIX
#endif

  if (time == static_cast<std::time_t>(-1)) {
    throw std::runtime_error("Invalid time value: " + iso_str);
  }

  return std::chrono::system_clock::from_time_t(time);
}

}  // namespace reduct

#define REDUCTCPP_TIME_PARSE_H

#endif  // REDUCTCPP_TIME_PARSE_H
