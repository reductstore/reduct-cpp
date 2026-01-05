// Copyright 2024 ReductSoftware UG

#ifndef REDUCT_CPP_DIAGNOSTICS_H
#define REDUCT_CPP_DIAGNOSTICS_H

#include <map>
#include <string>

namespace reduct {

struct DiagnosticsError {
  uint64_t count;            // Count of errors
  std::string last_message;  // Last error message

  auto operator<=>(const DiagnosticsError&) const = default;
};

struct DiagnosticsItem {
  uint64_t ok;                                 // Count of successful operations
  uint64_t errored;                            // Count of errored operations
  std::map<int16_t, DiagnosticsError> errors;  // Map of error codes to DiagnosticsError

  bool operator==(const DiagnosticsItem&) const = default;
};

struct Diagnostics {
  DiagnosticsItem hourly;  // Hourly diagnostics item

  bool operator==(const Diagnostics&) const = default;
};

}  // namespace reduct
#endif  // REDUCT_CPP_DIAGNOSTICS_H
