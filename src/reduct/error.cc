// Copyright 2022 Alexey Timin

#include "reduct/error.h"

#include <fmt/core.h>

namespace reduct {

const Error Error::kOk = Error{};

Error::operator bool() const { return code != 0; }

std::string Error::ToString() const { return fmt::format("[{}] {}", code, message); }

std::ostream& operator<<(std::ostream& os, const Error& error) {
  os << error.ToString();
  return os;
}

}  // namespace reduct
