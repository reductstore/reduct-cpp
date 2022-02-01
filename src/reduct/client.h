// Copyright 2022 Alexey Timin

#ifndef REDUCT_CPP_CLIENT_H
#define REDUCT_CPP_CLIENT_H

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "reduct/error.h"

namespace reduct {

/**
 * Result with error request
 */
template <typename T>
struct Result {
  T result;
  Error error;  // error code is HTTP status or -1 if it is communication error
};

/**
 * Result with error request
 */
template <typename T>
struct UPtrResult {
  std::unique_ptr<T> result;
  Error error;  // error code is HTTP status or -1 if it is communication error
};

class IBucket {
 public:
  enum class QuotaType { kNone, kFifo };

  struct Settings {
    std::optional<size_t> max_block_size;
    std::optional<QuotaType> quota_type;
    std::optional<size_t> quota_size;
  };

  using Time = std::chrono::time_point<std::chrono::system_clock>;

  struct ReadResult {
    std::string data;
    Time time;
    Error error;
  };

  struct RecordInfo {
    Time timestamp;
    size_t size{};
  };

  struct ListResult {
    std::vector<RecordInfo> records;
    Error error;
  };

  virtual ReadResult Read(std::string_view entry_name, Time ts) const = 0;
  virtual Error Write(std::string_view entry_name, std::string_view data, Time ts = Time::clock::now()) const = 0;
  virtual ListResult List(std::string_view entry_name, Time start, Time stop) const = 0;
  virtual Error Remove() const = 0;
};

class IClient {
 public:
  /**
   * Reduct Storage Information
   */
  struct ServerInfo {
    std::string version;  //  version of storage
    size_t bucket_count;  //  number of buckets

    bool operator<=>(const IClient::ServerInfo&) const = default;
  };

  /**
   * @brief Get information about the server
   * @return result and error
   */
  virtual Result<ServerInfo> GetInfo() const noexcept = 0;

  virtual UPtrResult<IBucket> GetBucket(std::string_view name) const noexcept = 0;
  virtual UPtrResult<IBucket> CreateBucket(std::string_view name, IBucket::Settings settings = {}) const noexcept = 0;

  /**
   * @brief Build a client
   * @param url URL of React Storage
   * @return
   */
  static std::unique_ptr<IClient> Build(std::string_view url);
};
}  // namespace reduct

#endif  // REDUCT_CPP_CLIENT_H
