# 📒 API Reference

The API reference for the SDK provides information on how to communicate
with [ReductStore via HTTP]((https://reduct.store/docs). The SDK is designed
to hide implementation details in .cc files, allowing users to work with abstract interfaces and factory methods.

## Error Handling

Error handling in the SDK is done through the use of `Error` or `Result` objects, rather than exceptions. These objects
contain the result of a function as well as an `Error` object, which can be checked for success or failure. An example of
error handling with the SDK:

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

Interface to manage buckets, tokens and the storage.

[IClient](iclient.md){ .md-button }

## IBucket

Interface to write, read and browse data in a bucket

[IBucket](ibucket.md){ .md-button }
