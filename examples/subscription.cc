// Copyright 2023 Alexey Timin

#include <reduct/client.h>

#include <iostream>
#include <thread>

using reduct::IBucket;
using reduct::IClient;

int main() {
  auto writer = std::thread([]() {
    auto client = IClient::Build("http://127.0.0.1:8383");

    auto [bucket, err] = client->GetOrCreateBucket("bucket");
    if (err) {
      std::cerr << "Error: " << err;
      return;
    }

    for (int i = 0; i < 10; ++i) {
      const IBucket::WriteOptions opts{
          .timestamp = IBucket::Time::clock::now(),
          .labels = {{"good", i % 2 == 0 ? "true" : "false"}},
      };

      const auto msg = "Hey " + std::to_string(i);
      [[maybe_unused]] auto write_err = bucket->Write("entry-1", opts, [msg](auto rec) { rec->WriteAll(msg); });
      std::cout << "Write: " << msg << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  });

  // Subscribe to good messages
  int good_count = 0;
  auto client = IClient::Build("http://127.0.0.1:8383");
  auto [bucket, err] = client->GetOrCreateBucket("bucket");
  if (err) {
    std::cerr << "Error: " << err;
    return -1;
  }

  const auto opts = IBucket::QueryOptions{
      .include = {{"good", "true"}},
      .continuous = true,
      .poll_interval = std::chrono::milliseconds{100},
  };

  // Continuously read messages until we get 3 good ones
  auto query_err =
      bucket->Query("entry-1", IBucket::Time::clock::now(), std::nullopt, opts, [&good_count](auto &&record) {
        auto [msg, read_err] = record.ReadAll();
        if (read_err) {
          std::cerr << "Error: " << read_err;
          return false;
        }
        std::cout << "Read: " << msg << std::endl;
        return ++good_count != 3;
      });

  writer.join();

  if (query_err) {
    std::cerr << "Query error:" << query_err;
    return -1;
  }
}
