# Usage Examples

## Bucket and Entry Status

Starting from ReductStore v1.18, buckets and entries have a `status` field that indicates their current state:
- `IBucket::Status::kReady` - The bucket or entry is ready for operations
- `IBucket::Status::kDeleting` - The bucket or entry is being deleted in the background

### Non-blocking Deletions

When you delete a bucket or entry, the deletion happens in the background. During this time:
- The status field will be set to `IBucket::Status::kDeleting`
- Any read, write, or delete operations will return HTTP 409 (Conflict)
- You can poll the status to wait for deletion to complete

Example:

```cpp
#include <reduct/client.h>
#include <iostream>
#include <thread>

using reduct::IBucket;
using reduct::IClient;

int main() {
    auto client = IClient::Build("http://127.0.0.1:8383");
    
    // Create a bucket
    auto [bucket, err] = client->GetOrCreateBucket("my-bucket");
    if (err) {
        std::cerr << "Error: " << err << std::endl;
        return -1;
    }
    
    // Check bucket status
    auto [info, info_err] = bucket->GetInfo();
    if (info_err) {
        std::cerr << "Error: " << info_err << std::endl;
        return -1;
    }
    
    std::cout << "Bucket status: " 
              << (info.status == IBucket::Status::kReady ? "READY" : "DELETING") 
              << std::endl;
    
    // Delete the bucket (non-blocking)
    err = bucket->Remove();
    if (err) {
        std::cerr << "Error: " << err << std::endl;
        return -1;
    }
    
    // Wait for deletion to complete
    bool deleted = false;
    while (!deleted) {
        auto [buckets, list_err] = client->GetBucketList();
        if (list_err) {
            std::cerr << "Error: " << list_err << std::endl;
            return -1;
        }
        
        deleted = true;
        for (const auto& bucket_info : buckets) {
            if (bucket_info.name == "my-bucket") {
                if (bucket_info.status == IBucket::Status::kDeleting) {
                    std::cout << "Bucket is still being deleted..." << std::endl;
                    deleted = false;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    break;
                }
            }
        }
    }
    
    std::cout << "Bucket deleted successfully" << std::endl;
    return 0;
}
```

### Handling HTTP 409 Conflicts

If you attempt operations on a bucket or entry that is being deleted, you'll receive HTTP 409:

```cpp
// This will return HTTP 409 if the bucket is being deleted
auto write_err = bucket->Write("entry", std::nullopt, [](auto rec) {
    rec->WriteAll("data");
});

if (write_err.code == 409) {
    std::cerr << "Bucket is being deleted, cannot write" << std::endl;
}
```
