// Copyright 2022-2024 Alexey Timin

#ifndef REDUCT_CPP_CLIENT_H
#define REDUCT_CPP_CLIENT_H

#include <chrono>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "diagnostics.h"
#include "reduct/bucket.h"
#include "reduct/error.h"
#include "reduct/http_options.h"
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
    std::string version;          //  version of storage
    size_t bucket_count;          //  number of buckets
    size_t usage;                 //  disk usage in bytes
    std::chrono::seconds uptime;  // server uptime
    Time oldest_record;
    Time latest_record;

    /**
     * @brief License information
     */
    struct License {
      std::string licensee;     // Licensee usually is the company name
      std::string invoice;      // Invoice number
      Time expiry_date;         // Expiry date of the license in ISO 8601 format (UTC)
      std::string plan;         // Plan name
      uint64_t device_number;   // Number of devices (0 for unlimited)
      uint64_t disk_quota;      // Disk quota in TB (0 for unlimited)
      std::string fingerprint;  // License fingerprint

      auto operator<=>(const License&) const = default;
    };

    std::optional<License> license;  // License information. If empty, then it is BUSL-1.1

    struct Defaults {
      IBucket::Settings bucket;  // default settings for a new bucket

      auto operator<=>(const IClient::ServerInfo::Defaults&) const = default;
    };

    Defaults defaults;

    auto operator<=>(const IClient::ServerInfo&) const = default;
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
   * Create a new bucket
   * @param name unique name for bucket
   * @param settings optional settings
   * @return pointer to created bucket
   */
  virtual UPtrResult<IBucket> CreateBucket(std::string_view name, IBucket::Settings settings = {}) const noexcept = 0;

  /**
   * Get an existing bucket or create a new bucket
   * @param name unique name for bucket
   * @param settings optional settings
   * @return pointer to created bucket
   */
  virtual UPtrResult<IBucket> GetOrCreateBucket(std::string_view name,
                                                IBucket::Settings settings = {}) const noexcept = 0;

  /**
   * API Token for authentication
   */
  struct Token {
    std::string name;  // name of token
    Time created_at;   // creation time
    bool is_provisioned;  // true if token is provisioned, you can't remove it or change its permissions


    auto operator<=>(const IClient::Token&) const = default;
  };

  /**
   * Token permissions
   */
  struct Permissions {
    bool full_access = false;        // full access to manage tokens and buckets
    std::vector<std::string> read;   // list of buckets with read access
    std::vector<std::string> write;  // list of buckets with write access

    auto operator<=>(const IClient::Permissions&) const = default;
  };

  /**
   * Token with permissions
   */
  struct FullTokenInfo {
    std::string name;     // name of token
    Time created_at;      // creation time
    bool is_provisioned;  // true if token is provisioned, you can't remove it or change its permissions

    Permissions permissions;

    auto operator<=>(const IClient::FullTokenInfo&) const = default;
  };

  /**
   * @brief Get list of tokens
   * @return the list or an error
   */
  [[nodiscard]] virtual Result<std::vector<Token>> GetTokenList() const noexcept = 0;

  /**
   * @brief Get information about token
   * @param name name of token
   * @return token info or an error
   */
  [[nodiscard]] virtual Result<FullTokenInfo> GetToken(std::string_view name) const noexcept = 0;

  /**
   * @brief Create a new token
   * @param name name of token
   * @param permissions permissions for token
   * @return token value or an error
   */
  [[nodiscard]] virtual Result<std::string> CreateToken(std::string_view name,
                                                        Permissions permissions) const noexcept = 0;

  /**
   * @brief Update token permissions
   * @param name name of token
   * @param permissions permissions for token
   * @return  an error
   */
  virtual Error RemoveToken(std::string_view name) const noexcept = 0;

  /**
   * @brief Returns information about the currently authenticated token.
   *
   * Makes a GET request to the '/me' endpoint using the client object stored as a member variable.
   *
   * @return A Result object containing a FullTokenInfo object or an Error object.
   */
  [[nodiscard]] virtual Result<FullTokenInfo> Me() const noexcept = 0;

  /**
   * Replication information
   */
  struct ReplicationInfo {
    std::string name;          // Replication name
    bool is_active;            // Remote instance is available and replication is active
    bool is_provisioned;       // Replication settings
    uint64_t pending_records;  // Number of records pending replication

    auto operator<=>(const ReplicationInfo&) const = default;
  };

  /**
   * Replication settings
   */
  struct ReplicationSettings {
    std::string src_bucket;  // Source bucket
    std::string dst_bucket;  // Destination bucket
    std::string dst_host;    // Destination host URL (e.g. https://reductstore.com)
    std::string dst_token;   // Destination access token
    std::vector<std::string>
        entries;                // Entries to replicate. If empty, all entries are replicated. Wildcards are supported.
    IBucket::LabelMap include;  // Labels to include
    IBucket::LabelMap exclude;  // Labels to exclude
    std::optional<double> each_s;  // Replicate a record every S seconds if not empty
    std::optional<uint64_t> each_n;  // Replicate every Nth record if not empty

    auto operator<=>(const ReplicationSettings&) const = default;
  };

  /**
   * Replication full info with settings and diagnostics
   */
  struct FullReplicationInfo {
    ReplicationInfo info;          // Replication info
    ReplicationSettings settings;  // Replication settings
    Diagnostics diagnostics;       // Diagnostics

    bool operator==(const FullReplicationInfo&) const = default;
  };

  /**
   * @brief Get list of replications
   * @return the list or an error
   */
  [[nodiscard]] virtual Result<std::vector<ReplicationInfo>> GetReplicationList() const noexcept = 0;

  /**
   * @brief Get replication info with settings and diagnostics
   * @param name name of replication
   * @return the info or an error
   */
  [[nodiscard]] virtual Result<FullReplicationInfo> GetReplication(std::string_view name) const noexcept = 0;

  /**
   * @brief Create a new replication
   * @param name name of replication
   * @param settings replication settings
   * @return error
   */
  [[nodiscard]] virtual Error CreateReplication(std::string_view name, ReplicationSettings settings) const noexcept = 0;

  /**
   * @brief Update replication settings
   * @param name name of replication
   * @param settings replication settings
   * @return error
   */
  [[nodiscard]] virtual Error UpdateReplication(std::string_view name, ReplicationSettings settings) const noexcept = 0;

  /**
   * @brief Remove replication
   * @param name name of replication
   * @return error
   */
  [[nodiscard]] virtual Error RemoveReplication(std::string_view name) const noexcept = 0;

  /**
   * @brief Build a client
   * @param url URL of React Storage
   * @return
   */
  static std::unique_ptr<IClient> Build(std::string_view url, HttpOptions options = {}) noexcept;
};
}  // namespace reduct

#endif  // REDUCT_CPP_CLIENT_H
