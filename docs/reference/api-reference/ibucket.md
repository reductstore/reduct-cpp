---
description: Interface to read, write and browse historical data
---

# IBucket

### Factory Methods

To get access to a bucket, you should use `IClient::GetBucket` or `IClient::CreateBucket`methods. See [IClient](iclient.md) interface.

### Read

You can read data from a specific entry of the bucket by using `IBucket::Read method:`

```cpp
//ts is std::chrono::system_clock::timepoint some known timestamp of the record
auto [blob, read_err] = bucket->Read("entry-1", ts);
if (!read_err) {
  std::cout << "Read blob: " <<blob << std::endl;
}
```

To read data, you should specify a name of the entry(`entry-1)` and an exact timestamp of the record. If you don't know the timestamps, use method`IBucket::List` to browse records.

### Write

The interface provide&#x20;
