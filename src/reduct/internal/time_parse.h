// Copyright 2025 ReductSoftware UG

#ifndef REDUCTCPP_TIME_PARSE_H
#define REDUCTCPP_TIME_PARSE_H

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

namespace reduct {

// Parse ISO8601 like "2024-10-11T13:45:30Z" or "2024-10-11T13:45:30.123456Z"
inline std::chrono::system_clock::time_point parse_iso8601_utc(const std::string& iso_str) {
  // Must end with 'Z' or 'z'
  if (iso_str.empty() || (iso_str.back() != 'Z' && iso_str.back() != 'z')) {
    throw std::runtime_error("Invalid timestamp (missing 'Z'): " + iso_str);
  }

  const std::string clean = iso_str.substr(0, iso_str.size() - 1);
  const std::size_t dot_pos = clean.find('.');
  const std::string time_part = clean.substr(0, dot_pos);

  std::tm tm = {};
  std::istringstream ss(time_part);
  ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
  if (ss.fail()) {
    throw std::runtime_error("Invalid timestamp format: " + iso_str);
  }

#if defined(_WIN32)
  std::time_t time = _mkgmtime(&tm);  // Windows
#else
  std::time_t time = timegm(&tm);  // POSIX
#endif

  if (time == static_cast<std::time_t>(-1)) {
    throw std::runtime_error("Invalid time value: " + iso_str);
  }

  auto parsed_time = std::chrono::system_clock::from_time_t(time);

  if (dot_pos == std::string::npos) {
    return parsed_time;
  }

  auto fraction = clean.substr(dot_pos + 1);
  if (fraction.empty()) {
    throw std::runtime_error("Invalid fractional timestamp: " + iso_str);
  }

  if (fraction.size() > 6) {
    fraction.resize(6);
  } else {
    fraction.append(6 - fraction.size(), '0');
  }

  if (fraction.find_first_not_of("0123456789") != std::string::npos) {
    throw std::runtime_error("Invalid fractional timestamp: " + iso_str);
  }

  return parsed_time + std::chrono::microseconds(std::stoll(fraction));
}

}  // namespace reduct

#endif  // REDUCTCPP_TIME_PARSE_H
