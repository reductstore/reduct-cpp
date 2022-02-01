// Copyright 2022 Alexey Timin

#ifndef REDUCT_CPP_HELPERS_H
#define REDUCT_CPP_HELPERS_H

#include <cstdlib>

struct StorageLauncher {
  StorageLauncher() {
    std::system(
        "docker run -p 8383:8383 --rm -d --name=reduct-cpp-container ghcr.io/reduct-storage/reduct-storage:main");
  }

  ~StorageLauncher() { std::system("docker stop reduct-cpp-container"); }
};

#endif  // REDUCT_CPP_HELPERS_H
