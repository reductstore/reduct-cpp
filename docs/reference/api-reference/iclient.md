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

To build a client, you should use `IClient::Build` static factory method with HTTP URL of the storage and options:

```cpp
IClient::Options opts {
    .api_token = "SOME_API_TOKEN",    // leave empty to use anonymous access
};

std::unique_ptr<IClient> client = IClient::Build("http://127.0.0.1:8383", opts);
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

See `IClient::ServerInfo` structure to find that information about the storage can be retrieved.

### GetBucketList

To get a list of buckets and their statistic information, you can use `GetBucketList` method:

```
auto [list, err] = client->GetBucketList();
for (auto& bucket : list) {
    std::cout << bucket.name;
} 
```

See `IClient::BucketInfo` structure for more information.

### CreateBucket

To create a new bucket in the storage, you should use `CreateBucket` method with `IBucket::Settings`:

```cpp
auto [bucket, err] =
    client->CreateBucket("bucket", IBucket::Settings{.quota_type = IBucket::QuotaType::kFifo, .quota_size = 1000000});
if (err) {
  std::cerr << "Error: " << err;
  return;
}

std::cout << bucket->GetSettings(); // bucket has type std::unique_ptr<IBucket>
```

You don't need to specify all the settings, if something is missed, the storage will use the default parameters.

### GetBucket

To work with an existing bucket, you should get by using `GetBucket` method:

```
auto [bucket, err] = client->Get("bucket");
if (err) {
  std::cerr << "Error: " << err;
  return;
}

std::cout << bucket->GetSettings(); // bucket has type std::unique_ptr<IBucket>
```
