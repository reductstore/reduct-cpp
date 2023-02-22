---
description: Interface to read, write and browse historical data
---

# IBucket

### Factory Methods

To get access to a bucket, you should use `IClient::GetBucket` or `IClient::CreateBucket`methods.
See [IClient](iclient.md) interface.

### Read

You can read data from a specific entry of a bucket by using the `IBucket::Read` method:

```cpp
//ts is std::chrono::system_clock::timepoint some known timestamp of the record
auto read_err = bucket->Read("entry-1", ts, [](auto record) {
    std::cout << record->ReadAll() << std::endl;
});
```


To read data, you should specify a name of a entry `entry-1` and an exact timestamp of a record. Also, you can take the latest record:

```cpp
auto read_err = bucket->Read("entry-1", std::nullopt, [](auto record) {
    std::cout << record->ReadAll() << std::endl;
});
```

If you need to receive a big blob of data, you can receive it in chunks:

```cpp
std::ofstream file("some.blob");
auto read_err = bucket->Read("entry-1", ts, [](auto record) {
     auto record_err = record.Read([&received](auto data) {
      file << data;
      return true;
    });
});
```


### Write

The interface provides method `IClient::Write` to write record to an entry:

```cpp
IBucket::Time time = IBucket::Time::clock::now();
[[maybe_unused]] auto write_err = bucket->Write("entry-1", time, [](auto rec) { rec->WriteAll("some_data1"); });
```

You also can write a big blob in chunks:

```cpp
const std::string blob(10'000, 'x');
bucket->Write("entry", time, [](auto rec) {
    rec->Write(blob.size(), [&blob](auto offset, auto size) {
        return std::pair{true, blob.substr(offset, size)};
    });
});
```

Additionally, you can specify the content type of the record and labels:

```cpp
bucket->Write("entry", IBucket::WriteOptions{
                            .timestamp = ts,
                            .labels = {{"label1", "value1"}, {"label2", "value2"}},
                            .content_type = "text/plain",
                        },
                        [&blob](auto rec) { rec->WriteAll("Hey!"); })
```


### Query

The `IClient::Query` method allows to iterate records for a time interval:

```cpp
  err = bucket->Query("entry-1", start, IBucket::Time::clock::now(), {}, [](auto&& record) {
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
  err = bucket->Query("entry-1", std::nullopt, std::nullopt, {}, [](auto&& record) {
      // ...
  });
```

The callback for each record is executed in a separated thread, so you should care of thread safety.
If you use labels, you can filter records by them:

```cpp
  err = bucket->Query("entry-1", start, IBucket::Time::clock::now(),
                      IBucket::QueryOptions{.include = {{"label1", "value1"}}, .exlude={{"label2", "value2"}}}, [](auto&& record) {
      // ...
  });
```

### Remove

You can remove a bucket by using `IBucket::Remove` method.

!!!warning When you remove a bucket, you remove all the data in it.
