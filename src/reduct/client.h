// Copyright 2022-2024 ReductSoftware UG

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
    std::string name;                  // name of token
    Time created_at;                   // creation time
    bool is_provisioned;               // true if token is provisioned, you can't remove it or change its permissions
    std::optional<Time> expires_at;    // absolute expiry timestamp (UTC)
    std::optional<uint64_t> ttl;       // inactivity TTL in seconds
    std::optional<Time> last_access;   // last access timestamp
    std::vector<std::string> ip_allowlist;  // allowed source IP addresses
    bool is_expired = false;           // token cannot be used anymore

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
    std::optional<Time> expires_at;    // absolute expiry timestamp (UTC)
    std::optional<uint64_t> ttl;       // inactivity TTL in seconds
    std::optional<Time> last_access;   // last access timestamp
    std::vector<std::string> ip_allowlist;  // allowed source IP addresses
    bool is_expired = false;           // token cannot be used anymore

    Permissions permissions;

    auto operator<=>(const IClient::FullTokenInfo&) const = default;
  };

  struct TokenCreateRequest {
    Permissions permissions;
    std::optional<Time> expires_at;
    std::optional<uint64_t> ttl;
    std::vector<std::string> ip_allowlist;

    auto operator<=>(const IClient::TokenCreateRequest&) const = default;
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
                                                        TokenCreateRequest request) const noexcept = 0;

  [[deprecated("Use CreateToken(name, TokenCreateRequest) to set ttl/expires_at/ip_allowlist")]]
  [[nodiscard]] virtual Result<std::string> CreateToken(std::string_view name,
                                                        Permissions permissions) const noexcept = 0;

  /**
   * @brief Update token permissions
   * @param name name of token
   * @param permissions permissions for token
   * @return  an error
   */
  virtual Error RemoveToken(std::string_view name) const noexcept = 0;

  [[nodiscard]] virtual Result<std::string> RotateToken(std::string_view name) const noexcept = 0;

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
  enum class ReplicationMode { kEnabled, kPaused, kDisabled };

  struct ReplicationInfo {
    std::string name;          // Replication name
    ReplicationMode mode = ReplicationMode::kEnabled;      // Replication mode
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
    std::optional<std::string> dst_token;   // Destination access token
    std::vector<std::string>
        entries;                // Entries to replicate. If empty, all entries are replicated. Wildcards are supported.
    std::optional<std::string> when;  // Replication condition
    ReplicationMode mode = ReplicationMode::kEnabled;  // Replication mode

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
   * @brief Update replication mode without changing settings
   * @param name name of replication
   * @param mode replication mode
   * @return error
   */
  [[nodiscard]] virtual Error SetReplicationMode(std::string_view name, ReplicationMode mode) const noexcept = 0;

  /**
   * @brief Remove replication
   * @param name name of replication
   * @return error
   */
  [[nodiscard]] virtual Error RemoveReplication(std::string_view name) const noexcept = 0;

  /**
   * Lifecycle information
   */
  enum class LifecycleType { kDelete };

  enum class LifecycleMode { kEnabled, kDisabled, kDryRun };

  struct LifecycleInfo {
    std::string name;    // Lifecycle name
    LifecycleMode mode = LifecycleMode::kEnabled;  // Lifecycle mode
    bool is_provisioned; // Lifecycle is provisioned
    bool is_running;     // Lifecycle worker is running

    auto operator<=>(const LifecycleInfo&) const = default;
  };

  /**
   * Lifecycle settings
   */
  struct LifecycleSettings {
    LifecycleType type = LifecycleType::kDelete;  // Lifecycle action type
    std::string bucket;         // Bucket to apply lifecycle policy
    std::vector<std::string> entries;  // Entries to process. If empty, all matching entries are used.
    std::string max_age;        // Maximum record age
    std::optional<std::string> interval;  // Interval between lifecycle runs
    std::optional<std::string> when;      // Lifecycle condition
    LifecycleMode mode = LifecycleMode::kEnabled;  // Lifecycle mode

    auto operator<=>(const LifecycleSettings&) const = default;
  };

  /**
   * Lifecycle full info with settings
   */
  struct FullLifecycleInfo {
    LifecycleInfo info;          // Lifecycle info
    LifecycleSettings settings;  // Lifecycle settings

    bool operator==(const FullLifecycleInfo&) const = default;
  };

  /**
   * @brief Get list of lifecycles
   * @return the list or an error
   */
  [[nodiscard]] virtual Result<std::vector<LifecycleInfo>> GetLifecycleList() const noexcept = 0;

  /**
   * @brief Get lifecycle info with settings
   * @param name name of lifecycle
   * @return the info or an error
   */
  [[nodiscard]] virtual Result<FullLifecycleInfo> GetLifecycle(std::string_view name) const noexcept = 0;

  /**
   * @brief Create a new lifecycle
   * @param name name of lifecycle
   * @param settings lifecycle settings
   * @return error
   */
  [[nodiscard]] virtual Error CreateLifecycle(std::string_view name, LifecycleSettings settings) const noexcept = 0;

  /**
   * @brief Update lifecycle settings
   * @param name name of lifecycle
   * @param settings lifecycle settings
   * @return error
   */
  [[nodiscard]] virtual Error UpdateLifecycle(std::string_view name, LifecycleSettings settings) const noexcept = 0;

  /**
   * @brief Update lifecycle mode without changing settings
   * @param name name of lifecycle
   * @param mode lifecycle mode
   * @return error
   */
  [[nodiscard]] virtual Error SetLifecycleMode(std::string_view name, LifecycleMode mode) const noexcept = 0;

  /**
   * @brief Remove lifecycle
   * @param name name of lifecycle
   * @return error
   */
  [[nodiscard]] virtual Error RemoveLifecycle(std::string_view name) const noexcept = 0;

  /**
   * @brief Build a client
   * @param url URL of React Storage
   * @return
   */
  static std::unique_ptr<IClient> Build(std::string_view url, HttpOptions options = {}) noexcept;
};
}  // namespace reduct

#endif  // REDUCT_CPP_CLIENT_H
