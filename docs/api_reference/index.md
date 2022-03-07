# 📒 API Reference

The SDK has a few interfaces to communicate with Reduct Storage via HTTP.&#x20;

!!! info The SDK is written in a way to hide all dependencies and implementation in .cc files. So, the user works only
with abstract interfaces and factory methods.;

## Error Handling

The SDK doesn't use exceptions for error handling. Instead of them, all methods return `Error` or `Result`, which
contains the result of function and `Error`.

The example of the error handling:

```cpp
reduct::Result<int> Foo() {
  return { 100, reduct::Error{.code=-1, .message="Something wrong"}};
}

auto [ret, err] = Foo();
if (err) {
  std::cerr << err;
}

std::cout << ret;
```

## IClient

Interface to manage buckets and the storage

[iclient.md](iclient.md){ .md-button }

## IBucket

Interface to write, read and browse data in a bucket

[ibucket.md](ibucket.md){ .md-button }
