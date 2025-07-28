# ReductStore Client SDK for C++

[![GitHub release (latest SemVer)](https://img.shields.io/github/v/release/reductstore/reduct-cpp)]()
![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/reductstore/reduct-cpp/ci.yml?branch=main)

The ReductStore Client SDK for C++ is an open source client for [ReductStore](https://www.reduct.store) written
in C++20. It allows developers to easily interact with the database from their C++ applications.

## Features

* Written in C++20
* Support ReductStore https://www.reduct.store/docs/http-api
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

## Installing

### Dependencies
* GCC 11.2 or higher (support C++20)
* CMake >= 3.23
* OpenSSL >= 3.2.2
* fmt >= 11.0.2
* nlohmann_json >= 3.11.3
* httplib >= 0.16.0
* concurrentqueue >= 1.0.4
* date >= 3.0.1 (for `REDUCT_CPP_USE_STD_CHRONO=OFF`)
* zlib >= 1.3.1

```shell
# Ubuntu
sudo apt install libfmt-dev \
        nlohmann-json3-dev \
        libcpp-httplib-dev \
        libconcurrentqueue-dev \
        libhowardhinnant-date-dev \
        libssl-dev
```

To build the library, follow these steps:

```shell
git clone https://github.com/reductstore/reduct-cpp.git
cd reduct-cpp

# Run cmake configuration
cmake -S . -B build  # for linux
cmake -S . -B build -DREDUCT_CPP_USE_STD_CHRONO=ON  -DOPENSSL_ROOT_DIR="<path>"   # for windows

# Build
cmake --build build # for linux
cmake --build build --config Release # for windows

# Install
sudo cmake --install build
```

## CMake Integration
Now to use the SDK in your C++ project, you need to add the `find_package` to CMakeLists.txt:

```cmake
cmake_minimum_required(VERSION 3.22)

project(ReductCppExample)

find_package(reductcpp)

add_executable(quick_start quick_start.cc)
target_link_libraries(quick_start reductcpp::reductcpp)
```
### Using FetchContent
- Install only OpenSSL
- Use `FetchContent` to download the ReductStore C++ SDK and its dependencies automatically
```cmake
cmake_minimum_required(VERSION 3.22)

project(ReductCppExample)

include(FetchContent)

# Enable using fetchcontent for reductcpp dependencies
set(REDUCT_CPP_USE_FETCHCONTENT ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    reductcpp
        URL https://github.com/reductstore/reduct-cpp/archive/refs/tags/v1.15.0.zip
)
FetchContent_MakeAvailable(reductcpp)

add_executable(quick_start quick_start.cc)
target_link_libraries(quick_start reductcpp::reductcpp)
```

### Building with Conan

Alternatively, you can use Conan to install the dependencies and build the library:

```shell
conan install . --build=missing
cmake --preset conan-release -DREDUCT_CPP_USE_CONAN=ON
cmake --build --preset conan-release --config Release
```

For more examples, see the [Guides](https://reduct.store/docs/guides) section in the ReductStore documentation.

### Supported ReductStore Versions and  Backward Compatibility

The library is backward compatible with the previous versions. However, some methods have been deprecated and will be
removed in the future releases. Please refer to the [Changelog](CHANGELOG.md) for more details.
The SDK supports the following ReductStore API versions:

* v1.15
* v1.14
* v1.13

It can work with newer and older versions, but it is not guaranteed that all features will work as expected because
the API may change and some features may be deprecated or the SDK may not support them yet.
