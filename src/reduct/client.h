// Copyright 2022 Alexey Timin

#ifndef REDUCT_CPP_CLIENT_H
#define REDUCT_CPP_CLIENT_H

#include <chrono>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "reduct/bucket.h"
#include "reduct/error.h"
#include "reduct/result.h"
namespace reduct {
/**
 * Interface for Reduct Storage HTTP Client
 */
class IClient {
 public:
  using Time = std::chrono::time_point<std::chrono::system_clock>;

  /**
   * Reduct Storage Information
   */
  struct ServerInfo {
    std::string version;  //  version of storage
    size_t bucket_count;  //  number of buckets
    size_t usage;         //  disk usage in bytes
    std::chrono::seconds uptime; // server uptime
    Time oldest_record;
    Time latest_record;

    bool operator<=>(const IClient::ServerInfo&) const = default;
  };

  /**
   * @brief Get information about the server
   * @return
   */
  virtual Result<ServerInfo> GetInfo() const noexcept = 0;

  /**
   * @brief Get list of buckets with stats
   * @return the list or an error
   */
  virtual Result<std::vector<IBucket::BucketInfo>> GetBucketList() const noexcept = 0;

  /**
   * Get an existing bucket
   * @param name name of bucket
   * @return pointer to bucket or an error
   */
  virtual UPtrResult<IBucket> GetBucket(std::string_view name) const noexcept = 0;

  /**
   * @brief Creates a new bucket
   * @param name unique name for bucket
   * @param settings optional settings
   * @return pointer to created bucket
   */
  virtual UPtrResult<IBucket> CreateBucket(std::string_view name, IBucket::Settings settings = {}) const noexcept = 0;

  /**
   * @brief Build a client
   * @param url URL of React Storage
   * @return
   */
  static std::unique_ptr<IClient> Build(std::string_view url) noexcept;
};
}  // namespace reduct

#endif  // REDUCT_CPP_CLIENT_H
