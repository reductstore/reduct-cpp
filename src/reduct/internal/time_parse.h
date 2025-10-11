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
  std::tm tm = {};
  std::istringstream ss(iso_str);

  // Try to parse with or without trailing 'Z'
  if (iso_str.back() == 'Z' || iso_str.back() == 'z') {
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
  } else {
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
  }

  if (ss.fail()) {
    throw std::runtime_error("Failed to parse ISO 8601 timestamp: " + iso_str);
  }

#if defined(_WIN32)
  std::time_t time = _mkgmtime(&tm);  // Windows equivalent of timegm
#else
  std::time_t time = timegm(&tm);  // Converts tm (UTC) → time_t
#endif

  if (time == -1) {
    throw std::runtime_error("Invalid time value for timestamp: " + iso_str);
  }

  return std::chrono::system_clock::from_time_t(time);
}
}  // namespace reduct

#define REDUCTCPP_TIME_PARSE_H

#endif  // REDUCTCPP_TIME_PARSE_H
