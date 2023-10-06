# 💡 Quick Start

This guide will help you get started with the ReductStore C++ SDK.

## Requirements

The ReductStore C++ SDK is written in C++20 and uses CMake as a build system. To install it, you will need:

* C++ compiler with C++20 support (we use GCC-11.2)
* cmake 3.18 or higher
* conan 1.40 or higher (it is optional, but [**conan**](https://conan.io) ia convenient package manager)

Currently, we have only tested the SDK on Linux AMD64, but if you need to port it to another
operating system or platform, you can create an [issue](https://github.com/reductstore/reduct-cpp/issues/new/choose)
for assistance.

## Installing

To install the ReductStore C++ SDK, follow these steps:

```
git clone https://github.com/reductstore/reduct-cpp.git
cd reduct-cpp
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
sudo cmake --build . --target install
```

## Simple Application

Let's create a simple C++ application that connects to the storage, creates a bucket, and writes and reads some data. We
will need two files: `CMakeLists.txt` and `main.cc`.

```
touch CMakeLists.txt main.cc
```

Here is the `CMakeLists.txt` file:

```cmake title="CMakelists.txt"
--8<-- "./examples/CMakeLists.txt:"
```

And here is the code in `main.cc`:

```cpp title="main.cc"
--8<-- "./examples/usage_example.cc:"
```

To compile the example, use the following commands:

```
mkdir build && cd build
cmake ..
cmake --build .
```

Before you can run the example, you will need to start the storage as a Docker container:

```
docker run -p 8383:8383  reductstore/reductstore:latest
```

You can then run the example with the following command:

```
./usage-example
```

## Next Steps

You can find more detailed documentation and examples in [the Reference API section](docs/api_reference/). You can also
refer to the [ReductStore HTTP API](https://docs.reduct.store/http-api) documentation for a complete reference
of the available API calls.
