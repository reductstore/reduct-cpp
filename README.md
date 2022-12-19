# ReductStore Client SDK for C++

[![GitHub release (latest SemVer)](https://img.shields.io/github/v/release/reductstore/reduct-cpp)]()
![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/reductstore/reduct-cpp/ci.yml?branch=main)

The ReductStore Client SDK for C++ is an open source client for [ReductStore](https://reduct.store) written
in C++20. It allows developers to easily interact with the database from their C++ applications.

## Features

* Written in C++20
* Support ReductStore HTTP API v1.2
* Support HTTP and HTTPS protocols
* Exception free
* Support Linux AMD64

## Example

Here is a simple example of how to use the ReductStore Client SDK for C++ to create a bucket, write data to it, and
read the data back:

```cpp
#include <reduct/client.h>

#include <iostream>

using reduct::IBucket;
using reduct::IClient;

int main() {
  auto client = IClient::Build("https://play.reduct.store");
  // Create a bucket
  auto [bucket, create_err] = client->GetOrCreateBucket("bucket");
  if (create_err) {
    std::cerr << "Error: " << create_err;
    return -1;
  }

  // Write some data
  auto ts = IBucket::Time::clock::now();
  [[maybe_unused]] auto write_err = bucket->Write("entry-1", ts, [](auto rec) { rec->WriteAll("some_data1"); });

  // Read data via timestamp
  auto read_err = bucket->Read("entry-1", ts, [](auto rec) {
        std::cout << "Read blob: " <<  rec->ReadAll() << std::endl;
  });


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

To build the library, follow these steps:

```shell
git clone https://github.com/reductstore/reduct-cpp.git
cd reduct-cpp
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
sudo cmake --build . --target install
```

CMake tries to use the `conan` package manager if it is installed. If it isn't, it downloads all the dependencies by using
FetchContent. To use ReductStore SDK you need only to use `find_pacakge` in your cmake lists:

```cmake
find_package(ReductCpp)
```

## References

* [Documentation](https://cpp.reduct.store)
* [ReductStore HTTP API](https://docs.reduct.store/http-api)
