// Copyright 2022 Alexey Timin

#include <reduct/client.h>

#include <iostream>

using reduct::IBucket;
using reduct::IClient;

int main() {
  auto client = IClient::Build("http://127.0.0.1:8383/reduct/");

  // Get information about the server
  auto [info, err] = client->GetInfo();
  if (err) {
    std::cerr << "Error: " << err;
    return -1;
  }

  std::cout << "Server version: " << info.version;
  // Create a bucket
  auto [bucket, create_err] = client->GetOrCreateBucket("bucket");
  if (create_err) {
    std::cerr << "Error: " << create_err;
    return -1;
  }

  // Write some data
  IBucket::Time start = IBucket::Time::clock::now();
  [[maybe_unused]] auto write_err =
      bucket->Write("entry-1", std::nullopt, [](auto rec) { rec->WriteAll("some_data1"); });
  write_err = bucket->Write("entry-1", std::nullopt, [](auto rec) { rec->WriteAll("some_data2"); });
  write_err = bucket->Write("entry-1", std::nullopt, [](auto rec) { rec->WriteAll("some_data3"); });

  // Walk through the data
  err = bucket->Query("entry-1", start, IBucket::Time::clock::now(), {}, [](auto&& record) {
    std::string blob;
    auto read_err = record.Read([&blob](auto chunk) {
      blob.append(chunk);
      return true;
    });

    if (!read_err) {
      std::cout << "Read blob: " << blob;
    }

    return true;
  });
}
