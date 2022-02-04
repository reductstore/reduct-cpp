// Copyright 2022 Alexey Timin

#include <fmt/core.h>
#include <reduct/client.h>

using reduct::IBucket;
using reduct::IClient;

int main() {
  auto client = IClient::Build("http://127.0.0.1:8383");

  // Get information about the server
  auto [info, err] = client->GetInfo();
  if (err) {
    fmt::print("Error: {}\n", err.ToString());
    return -1;
  }

  fmt::print("Server version: {}\n", info.version);
  // Create a bucket
  auto [bucket, create_err] =
      client->CreateBucket("bucket", IBucket::Settings{.quota_type = IBucket::QuotaType::kFifo, .quota_size = 1000000});
  if (create_err) {
    fmt::print("Error: {}\n", create_err.ToString());
    return -1;
  }

  // Write some data
  IBucket::Time start = IBucket::Time::clock::now();
  [[maybe_unused]] auto write_err = bucket->Write("entry-1", "some_data1");
  write_err = bucket->Write("entry-1", "some_data2");
  write_err = bucket->Write("entry-1", "some_data3");

  // Walk through the data
  auto [records, _] = bucket->List("entry-1", start, IBucket::Time::clock::now());
  for (auto&& record : records) {
    auto [blob, read_err] = bucket->Read("entry-1", record.timestamp);
    if (!read_err) {
      fmt::print("Read blob: '{}'\n", blob);
    }
  }
}
