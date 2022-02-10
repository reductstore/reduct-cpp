# 📒 API Reference

The SDK has a few interfaces to communicate with Reduct Storage via HTTP.&#x20;

{% hint style="info" %}
The SDK is written in a way to hide all dependecies and implementation in .cc files. So, the user works only with abstract interfecaes and factory methods.&#x20;
{% endhint %}

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
