// Copyright 2022 ReductSoftware UG

#ifndef REDUCT_CPP_RESULT_H
#define REDUCT_CPP_RESULT_H

#include <memory>

#include "reduct/error.h"

namespace reduct {

/**
 * Result with error request
 */
template <typename T>
struct [[nodiscard]] Result {  // NOLINT
  T result;
  Error error;  // error code is HTTP status or -1 if it is communication error

  operator const Error&() const noexcept { return error; }
};

/**
 * Result with error request
 */
template <typename T>
struct [[nodiscard]] UPtrResult {  // NOLINT
  std::unique_ptr<T> result;
  Error error;  // error code is HTTP status or -1 if it is communication error

  operator const Error&() const noexcept { return error; }
  operator const std::unique_ptr<T>() noexcept { return std::move(result); }
};

}  // namespace reduct
#endif  // REDUCT_CPP_RESULT_H
