---
description: Interface to manage buckets and the storage
---

# IClient

### Usage Example

```cpp
using reduct::IClient;
using reduct::IBucket;

auto client = IClient::Build("http://127.0.0.1:8383");

// Get information about the server
auto [info, err] = client->GetInfo();
if (err) {
  std::cerr << "Error: " << err;
  return;
}

std::cout << "Server version: " << info.version;
// Create a bucket
auto [bucket, create_err] =
    client->CreateBucket("bucket", IBucket::Settings{.quota_type = IBucket::QuotaType::kFifo, .quota_size = 1000000});
if (create_err) {
  std::cerr << "Error: " << create_err;
  return;
}
```

### Factory

To build a client, you should use `IClient::Build` static factory method with HTTP URL of the storage:

```cpp
std::unique_ptr<IClient> client = IClient::Build("http://127.0.0.1:8383");
```



### GetInfo

You can take information about the storage by using method `GetInfo`:

```cpp
// Get information about the server
auto [info, err] = client->GetInfo();
if (err) {
  std::cerr << "Error: " << err;
  return;
}

std::cout << "Server version: " << info.version;
```

