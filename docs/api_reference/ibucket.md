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
[[maybe_unused]] auto write_err = bucket->Write("entry-1", ts, [](auto rec) { rec->WriteAll("some_data1"); });
```

You also can write a big blob by chunks:

```cpp
const std::string blob(10'000, 'x');
bucket->Write("entry", ts, [](auto rec) {
    rec->Write(blob.size(), [&blob](auto offset, auto size) {
        return std::pair{true, blob.substr(offset, size)};
    });
});
```

### Query

The `IClient::Query` method allows to iterate records for a time interval:

```cpp
  err = bucket->Query("entry-1", start, IBucket::Time::clock::now(), std::nullopt, [](auto&& record) {
    std::string blob;
    auto read_err = record.Read([&blob](auto chunk) {
      blob.append(chunk);
      return true;  // return false if you want to aboard the receiving
    });

    if (!read_err) {
      std::cout << "Read blob: " << blob;
    }

    return true; // return false if you want stop iterating
  });
```

You can walk through all the records in an entry if you don't specify the start and stop time points:

```cpp
  err = bucket->Query("entry-1", std::nullopt, std::nullopt, std::nullopt, [](auto&& record) {
      // ...
  });
```

The callback for each record is executed in a separated thread, so you should care of thread safety.

### Remove

You can remove a bucket by using `IBucket::Remove` method.

!!!warning When you remove a bucket, you remove all the data in it.
