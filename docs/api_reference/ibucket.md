---
description: Interface to read, write and browse historical data
---

# IBucket

### Factory Methods

To get access to a bucket, you should use `IClient::GetBucket` or `IClient::CreateBucket`methods.
See [IClient](iclient.md) interface.

### Read

You can read data from a specific entry of the bucket by using `IBucket::Read` method:

```cpp
//ts is std::chrono::system_clock::timepoint some known timestamp of the record
auto [blob, read_err] = bucket->Read("entry-1", ts);
if (!read_err) {
  std::cout << "Read blob: " <<blob << std::endl;
}
```

To read data, you should specify a name of the entry(`entry-1)` and an exact timestamp of the record. If you don't know
the timestamps, use method`IBucket::List` to browse records. Also, you can take the latest record don't specify the
timestamp:

```cpp
auto [latest_record, read_err] = bucket->Read("entry-1");
```

If you need to receive a big blob of data, you can receive it by chunks:

```cpp
std::ofstream file("some.blob");
auto err = bucket->Read("entry", ts, [&file](auto data) {
    file << data;
    return true;
});
```

### Write

The interface provides method`IClient::Write` to write some data to an entry:

```cpp
IBucket::Time ts = IBucket::Time::clock::now();
[[maybe_unused]] auto write_err = bucket->Write("entry-1", "some_data1", ts);
```

### List

`IClient::List`method returns a list of timestamps and sizes of records for some time interval:

```cpp
auto now = IBucket::Time::clock::now();
auto [records, _] = bucket->List("entry-1", now - std::chrono::hours(1), now);
for (auto&& record : records) {
  auto [blob, read_err] = bucket->Read("entry-1", record.timestamp);
  if (!read_err) {
    std::cout << "Read blob: " <<  blob;
  }
}
```

### Remove

You can remove a bucket by using `IBucket::Remove` method.

!!!warning When you remove a bucket, you remove all the data in it.
