// Copyright 2022 Alexey Timin

#ifndef REDUCT_ERROR_H
#define REDUCT_ERROR_H

#include <ostream>
#include <string>
#include <variant>

namespace reduct {

struct [[nodiscard]] Error { // NOLINT
  int code = 0;
  std::string message{};

  operator bool() const;

  std::string ToString() const;

  bool operator<=>(const Error& rhs) const  = default;
  friend std::ostream& operator<<(std::ostream& os, const Error& error);

  static const Error kOk;
};

}  // namespace reduct
#endif  // REDUCT_ERROR_H
