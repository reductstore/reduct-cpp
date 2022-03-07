# Reduct Storage Client SDK for C++

Open source client for [Reduct Storage](https://reduct-storage.dev) written in C++20.

## Features

* Implement Reduct Storage HTTP API v0.2.0
* Exception free
* Support Linux AMD64

## Example

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