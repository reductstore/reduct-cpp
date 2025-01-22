# ReductStore Client SDK for C++

[![GitHub release (latest SemVer)](https://img.shields.io/github/v/release/reductstore/reduct-cpp)]()
![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/reductstore/reduct-cpp/ci.yml?branch=main)

The ReductStore Client SDK for C++ is an open source client for [ReductStore](https://www.reduct.store) written
in C++20. It allows developers to easily interact with the database from their C++ applications.

## Features

* Written in C++20
* Support ReductStore [HTTP API v1.14](https://www.reduct.store/docs/next/http-api)
* Support HTTP and HTTPS protocols
* Support Linux AMD64 and Windows

## Example

Here is a simple example of how to use the ReductStore Client SDK for C++ to create a bucket, write data to it, and
read the data back:

```cpp
#include <reduct/client.h>

#include <iostream>
#include <cassert>

using reduct::IBucket;
using reduct::IClient;
using reduct::Error;
using sec = std::chrono::seconds;

int main() {
    // 1. Create a ReductStore client
    auto client = IClient::Build("http://127.0.0.1:8383",{
        .api_token = "my-token"
    });

    // 2. Get or create a bucket with 1Gb quota
    auto [bucket, create_err] = client->GetOrCreateBucket("my-bucket", {
        .quota_type = IBucket::QuotaType::kFifo,
        .quota_size = 1'000'000'000
    });

    if (create_err) {
        std::cerr << "Error: " << create_err;
        return -1;
    }

    // 3.  Write some data with timestamps and labels to the 'entry-1' entry
    IBucket::Time start = IBucket::Time::clock::now();
    auto write_err =
            bucket->Write("sensor-1", {
                .timestamp = start,
                .labels = {{"score", "10"}}
                }, [](auto rec) { rec->WriteAll("<Blob data>"); });
    assert(write_err == Error::kOk);

    write_err = bucket->Write("sensor-1", {
        .timestamp = start + sec(1),
        .labels = {{"score", "20"}}
    }, [](auto rec) { rec->WriteAll("<Blob data>"); });
    assert(write_err == Error::kOk);

    // 4. Query the data by time range and condition
    auto err = bucket->Query("sensor-1", start,  start + sec(2), {
            .when = R"({"&score": {"$gt": 10}})"

        }, [](auto&& record) {
        std::cout << "Timestamp: " << record.timestamp.time_since_epoch().count() << std::endl;
        std::cout << "Content Length: " << record.size << std::endl;

        auto [blob, read_err] = record.ReadAll();
        if (!read_err) {
            std::cout << "Read blob: " << blob << std::endl;
        }

        return true;
    });

    if (err) {
        std::cerr << "Error: " << err;
        return -1;
    }

    return 0;
}
```

## Build

* GCC 11.2 or higher (support C++20)
* CMake 3.18 or higher
* ZLib
* OpenSSL 1.1 or 3.0
* Conan >=2.0 (optionally)

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

For more examples, see the [Guides](https://reduct.store/docs/guides) section in the ReductStore documentation.
