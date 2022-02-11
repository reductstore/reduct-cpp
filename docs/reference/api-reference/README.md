# 📒 API Reference

The SDK has a few interfaces to communicate with Reduct Storage via HTTP.&#x20;

{% hint style="info" %}
The SDK is written in a way to hide all dependecies and implementation in .cc files. So, the user works only with abstract interfecaes and factory methods.&#x20;
{% endhint %}

## Error Handling

The SDK doesn't use exceptions for error handling. Instead of them, all methods return `Error` or `Result`, which contains the result of function and `Error`.

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

{% content-ref url="iclient.md" %}
[iclient.md](iclient.md)
{% endcontent-ref %}

## IBucket

Interface to write, read and browse data in a bucket

{% content-ref url="ibucket.md" %}
[ibucket.md](ibucket.md)
{% endcontent-ref %}
