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

    struct Defaults {
      IBucket::Settings bucket;  // default settings for a new bucket

      bool operator<=>(const IClient::ServerInfo::Defaults&) const = default;
    };

    Defaults defaults;

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

    bool operator<=>(const IClient::Token&) const = default;
  };

  /**
   * Token permissions
   */
  struct Permissions {
    bool full_access = false;        // full access to manage tokens and buckets
    std::vector<std::string> read;   // list of buckets with read access
    std::vector<std::string> write;  // list of buckets with write access

    bool operator<=>(const IClient::Permissions&) const = default;
  };

  /**
   * Token with permissions
   */
  struct FullTokenInfo {
    std::string name;  // name of token
    Time created_at;   // creation time
    bool is_provisioned;  // true if token is provisioned, you can't remove it or change its permissions

    Permissions permissions;

    bool operator<=>(const IClient::FullTokenInfo&) const = default;
  };

  /**
   * @brief Get list of tokens
   * @return the list or an error
   */
  virtual Result<std::vector<Token>> GetTokenList() const noexcept = 0;

  /**
   * @brief Get information about token
   * @param name name of token
   * @return token info or an error
   */
  virtual Result<FullTokenInfo> GetToken(std::string_view name) const noexcept = 0;

  /**
   * @brief Create a new token
   * @param name name of token
   * @param permissions permissions for token
   * @return token value or an error
   */
  virtual Result<std::string> CreateToken(std::string_view name, Permissions permissions) const noexcept = 0;

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
 *
 * Example:
 *
 * MyClass obj;
 * auto [token_info, err] = obj.Me();
 * if (err) {
 *   std::cerr << err << std::endl;
 * } else {
 *   std::cout << token_info.name << std::endl;
 * }
   */
  virtual Result<FullTokenInfo> Me() const noexcept = 0;

  /**
   * @brief Build a client
   * @param url URL of React Storage
   * @return
   */
  static std::unique_ptr<IClient> Build(std::string_view url, HttpOptions options = {}) noexcept;
};
}  // namespace reduct

#endif  // REDUCT_CPP_CLIENT_H
