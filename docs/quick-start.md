# 💡 Quick Start

## Requirements

Reduct Storage C++ SDK is written in C++20 and uses cmake as a build system. To install it, you need:

* C++ compiler with C++20 support (we use GCC-11.2)
* cmake 3.18 or higher
* conan 1.40 or higher (it is optional, but [**conan**](https://conan.io) is cool!)

Currently, we test only Linux AMD64, but if you need to port it to another OS or platform, feel free to make an [issue](https://github.com/reduct-storage/reduct-cpp/issues/new/choose).

## Installing

To install the SDK, you should use the classical approach:

```
git clone https://github.com/reduct-storage/reduct-cpp.git
cd reduct-cpp
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
sudo cmake --build . --target install
```

## Simple Application

Let's create a simple C++ application which connects to the storage, creates a bucket and writes/reads some data. We need two files:

```
touch CMakeLists.txt main.cc
```

I'm sure you're familiar to cmake, so I don't have to explain all the detail. The `CMakeLists.txt` should look like this:

```cmake
cmake_minimum_required(VERSION 3.18)

project(ReductCppExamples)
set(CMAKE_CXX_STANDARD 20)


find_package(ReductCpp 0.1.0)

add_executable(usage-example main.cc)
target_link_libraries(usage-example ${REDUCT_CPP_LIBRARIES})
```

And now the code in `main.cc`:

```cpp
#include <reduct/client.h>

#include <iostream>

using reduct::IBucket;
using reduct::IClient;

int main() {
  auto client = IClient::Build("http://127.0.0.1:8383");
  // Create a bucket
  auto [bucket, create_err] =
      client->CreateBucket("bucket");
  if (create_err) {
    std::cerr << "Error: " << create_err;
    return -1;
  }

  // Write some data
  IBucket::Time ts = IBucket::Time::clock::now();
  [[maybe_unused]] auto write_err = bucket->Write("entry-1", "some_data1", ts);

  // Read data
  auto [blob, read_err] = bucket->Read("entry-1", ts);
  if (!read_err) {
    std::cout << "Read blob: " <<  blob << std::endl;
  }
}
```

To compile the example, we use the same idiom:

```
mkdir build && cd build
cmake ..
cmake --build .
```

Let's run the code, but before you need to run the storage. For this, we have to run it as a Docker container:

```
docker run -p 8383:8383  ghcr.io/reduct-storage/reduct-storage:main
```

Now you can launch the example:

```
./usage-example
```
