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
* Conan and Vpkg package managers support

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

For more examples, including how to handle non-blocking deletions and bucket/entry status, see the [examples](examples/README.md) directory.

## Non-blocking Deletions (v1.18+)

Starting from ReductStore v1.18, bucket and entry deletions are non-blocking. When you delete a bucket or entry:

- Deletion happens in the background
- The `status` field in `BucketInfo` and `EntryInfo` indicates the state: `IBucket::Status::kReady` or `IBucket::Status::kDeleting`
- Operations on deleting buckets/entries return HTTP 409 (Conflict)

```cpp
// Check bucket status
auto [info, err] = bucket->GetInfo();
if (info.status == IBucket::Status::kDeleting) {
    std::cout << "Bucket is being deleted" << std::endl;
}

// List entries with status
auto [entries, list_err] = bucket->GetEntryList();
for (const auto& entry : entries) {
    if (entry.status == IBucket::Status::kDeleting) {
        std::cout << "Entry " << entry.name << " is being deleted" << std::endl;
    }
}
```

See [examples/README.md](examples/README.md) for detailed examples on handling non-blocking deletions.

## Integration

### Build and Install reduct-cpp system-wide

#### Dependencies

* GCC 11.2 or higher (support C++20)
* CMake >= 3.23
* OpenSSL >= 3.0.13
* fmt >= 11.0.2
* nlohmann_json >= 3.11.3
* httplib >= 0.16.0
* concurrentqueue >= 1.0.4

For Ubuntu, you can install the dependencies using the following command:

```shell
# Ubuntu
sudo apt install g++ cmake libssl-dev \
        libfmt-dev \
        nlohmann-json3-dev \
        libcpp-httplib-dev \
        libconcurrentqueue-dev \
        libhowardhinnant-date-dev
```

#### Build and Install

To build and install the ReductStore C++ SDK system-wide, you can use the following steps:

```shell
git clone https://github.com/reductstore/reduct-cpp.git
cd reduct-cpp

# Run cmake configuration
cmake -S . -B build  # for linux
cmake -S . -B build -DOPENSSL_ROOT_DIR="<path>"   # for windows

# Build
cmake --build build # for linux
cmake --build build --config Release # for windows

# Install
sudo cmake --install build
```

#### CMake Configuration

You can use the ReductStore C++ SDK in your CMake project by linking against the `reductcpp` target. Here is an example of how to do this:

```cmake
cmake_minimum_required(VERSION 3.23)

project(ReductCppExample)

find_package(reductcpp)

add_executable(quick_start quick_start.cc)
target_link_libraries(quick_start reductcpp::reductcpp)
```

### Using fetchContent (recommended)

If you want to use the ReductStore C++ in your project without installing it system-wide, you can use `FetchContent` to
download the ReductStore C++ SDK and its dependencies automatically. This way, you don't need to install the
dependencies (except OpenSSL)

#### Dependencies
* GCC 11.2 or higher (support C++20)
* CMake >= 3.23
* OpenSSL >= 3.2.2
```shell

# Ubuntu
sudo apt install g++ cmake libssl-dev
```

#### CMake Integration

You can use the following CMake code to integrate the ReductStore C++ SDK into your project using `FetchContent`:

```cmake
cmake_minimum_required(VERSION 3.23)

project(ReductCppExample)

include(FetchContent)

# Enable using fetchcontent for reductcpp dependencies
set(REDUCT_CPP_USE_FETCHCONTENT ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    reductcpp
        URL https://github.com/reductstore/reduct-cpp/archive/refs/tags/v1.16.0.zip
)
FetchContent_MakeAvailable(reductcpp)

add_executable(quick_start quick_start.cc)
target_link_libraries(quick_start reductcpp::reductcpp)
```

### Building with Conan

Alternatively, you can use Conan to install the dependencies and build the library:

```shell
conan install . --build=missing
cmake --preset conan-release
cmake --build --preset conan-release --config Release
```

### Examples

For more examples, see the [Guides](https://reduct.store/docs/guides) section in the ReductStore documentation.

### Supported ReductStore Versions and  Backward Compatibility

The library is backward compatible with the previous versions. However, some methods have been deprecated and will be
removed in the future releases. Please refer to the [Changelog](CHANGELOG.md) for more details.
The SDK supports the following ReductStore API versions:

* v1.18
* v1.17
* v1.16

It can work with newer and older versions, but it is not guaranteed that all features will work as expected because
the API may change and some features may be deprecated or the SDK may not support them yet.
