# Reduct Storage Client SDK for C++
![GitHub release (latest SemVer)](https://img.shields.io/github/v/release/reduct-storage/reduct-cpp)
![GitHub Workflow Status](https://img.shields.io/github/workflow/status/reduct-storage/reduct-cpp/ci)

Open source client for [Reduct Storage](https://reduct-storage.dev) written in C++20.

## Features

* Written in C++20
* Implement Reduct Storage HTTP API v1.0
* Support HTTP and HTTPS protocols
* Exception free
* Support Linux AMD64

## Example

```cpp
#include <reduct/client.h>

#include <iostream>

using reduct::IBucket;
using reduct::IClient;

int main() {
  auto client = IClient::Build("https://play.reduct-storage.dev");
  // Create a bucket
  auto [bucket, create_err] = client->GetOrCreateBucket("bucket");
  if (create_err) {
    std::cerr << "Error: " << create_err;
    return -1;
  }

  // Write some data
  IBucket::Time ts = IBucket::Time::clock::now();
  [[maybe_unused]] auto write_err = bucket->Write("entry-1", "some_data1", ts);

  // Read data via timestamp
  auto [blob, read_err] = bucket->Read("entry-1", ts);
  if (!read_err) {
    std::cout << "Read blob: " <<  blob << std::endl;
  }
  
  // Walk through the data
  err = bucket->Query("entry-1", std::nullopt, IBucket::Time::clock::now(), std::nullopt, [](auto&& record) {
    std::string blob;
    auto read_err = record.Read([&blob](auto chunk) {
      blob.append(chunk);
      return true;
    });

    if (!read_err) {
      std::cout << "Read blob: " << blob;
    }

    return true;
  });
}
```

## Build

* GCC 11.2 or higher (support C++20)
* CMake 3.18 or higher
* ZLib
* OpenSSL 1.1 or 3.0
* Conan 1.40 (optionally)

```shell
git clone https://github.com/reduct-storage/reduct-cpp.git
cd reduct-cpp
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
sudo cmake --build . --target install
```

CMake tries to use package manager `conan` if it is installed. If it isn't, it downloads all the dependencies by using
FetchContent. To use Reduct Storage SDK you need only to use `find_pacakge` in your cmake lists:

```cmake
find_package(ReductCpp)
```

## References

* [Documentation](https://cpp.reduct-storage.dev)
* [Reduct Storage HTTP API](https://docs.reduct-storage.dev/http-api)
