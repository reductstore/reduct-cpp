# Reduct Storage Client SDK for C++

Open source client for [Reduct Storage](https://reduct-storage.dev) written in C++20.

## Features

* Implement Reduct Storage HTTP API v0.1.0
* Exception free
* Tested on Linux AMD64

## Example

```cpp

#include <reduct/client.h>
#include <fmt/core.h>

using reduct::IClient;

int main() {
    auto client = IClient::Build("http://127.0.0.1:8383");
    
    auto [info, err] = client->GetInfo();
    if (err) {
        fmt::print("Error: {}", err.ToString());
        return -1;
    }
    
    fmt::print("Server version: {}", info.version);
    
    return 0;
}
```

## Requirements

* GCC 11.2 or higher
* CMake 3.18 or higher
* Conan 1.40 or higher

