// Copyright 2022 Alexey Timin

#ifndef REDUCT_CPP_HELPERS_H
#define REDUCT_CPP_HELPERS_H

#include <fmt/core.h>

#include <cstdlib>
#include <random>

inline std::string RandomBucketName() {
  std::random_device r;

  std::default_random_engine e1(r());
  std::uniform_int_distribution<int> uniform_dist(1, 10000000);

  return fmt::format("bucket_{}", uniform_dist(e1));
}

#endif  // REDUCT_CPP_HELPERS_H
