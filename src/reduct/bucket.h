// Copyright 2022 Alexey Timin
#ifndef REDUCT_CPP_BUCKET_H
#define REDUCT_CPP_BUCKET_H

#include <chrono>
#include <functional>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "reduct/error.h"
#include "reduct/http_options.h"
#include "reduct/result.h"

namespace reduct {

/**
 * Provides Bucket HTTP API
 */
class IBucket {
 public:
  enum class QuotaType { kNone, kFifo };

  /**
   * Bucket Settings
   */
  struct Settings {
    std::optional<size_t> max_block_size;
    std::optional<QuotaType> quota_type;
    std::optional<size_t> quota_size;

    bool operator<=>(const Settings&) const noexcept = default;
    friend std::ostream& operator<<(std::ostream& os, const Settings& settings);
  };

  using Time = std::chrono::time_point<std::chrono::system_clock>;

  /**
   * Stats of bucket
   */
  struct BucketInfo {
    std::string name;    // name of bucket
    size_t entry_count;  // number of entries in the bucket
    size_t size;         // size of stored data in the bucket in bytes
    Time oldest_record;  // timestamp of the oldest record in the bucket
    Time latest_record;  // timestamp of the latest record in the bucket

    bool operator<=>(const BucketInfo&) const noexcept = default;
    friend std::ostream& operator<<(std::ostream& os, const BucketInfo& info);
  };

  /**
   * Stats of entry
   */
  struct EntryInfo {
    std::string name;     // name of entry
    size_t record_count;  // number of entries in the entry
    size_t block_count;   // number of blocks in the entry
    size_t size;          // size of stored data in the bucket in bytes
    Time oldest_record;   // timestamp of the oldest record in the entry
    Time latest_record;   // timestamp of the latest record in the entry

    bool operator<=>(const EntryInfo&) const noexcept = default;
    friend std::ostream& operator<<(std::ostream& os, const EntryInfo& info);
  };

  /**
   * Information about a record
   */
  struct RecordInfo {
    Time timestamp;
    size_t size{};

    bool operator<=>(const RecordInfo&) const noexcept = default;
    friend std::ostream& operator<<(std::ostream& os, const RecordInfo& settings);
  };

  /**
   * Returns list of records for a time interval
   * @param entry_name name of the entry
   * @param start start time point in the interval
   * @param stop stop time point in the interval
   * @return the list or communication error
   */
  [[nodiscard]] virtual Result<std::vector<RecordInfo>> List(std::string_view entry_name, Time start,
                                                             Time stop) const noexcept = 0;

  /**
   * Write an object to the storage with timestamp
   * @param entry_name entry in bucket
   * @param data an object ot store
   * @param ts timestamp, if it is nullopt, if it is nullpot the method uses current time
   * @return HTTP or communication error
   */
  virtual Error Write(std::string_view entry_name, std::string_view data,
                      std::optional<Time> ts = std::nullopt) const noexcept = 0;

  /**
   * Read a record by timestamp
   * @param entry_name entry in bucket
   * @param ts timestamp, if it is nullopt, the method returns the latest record
   * @return HTTP or communication error
   */
  virtual Result<std::string> Read(std::string_view entry_name,
                                   std::optional<Time> ts = std::nullopt) const noexcept = 0;

  /**
   * @brief Get settings by HTTP request
   * @return settings or HTTP error
   */
  virtual Result<Settings> GetSettings() const noexcept = 0;

  /**
   * @brief Update settings which has values
   * @param settings
   * @return HTTP or communication error
   */
  virtual Error UpdateSettings(const Settings& settings) const noexcept = 0;

  /**
   * @brief Get stats about bucket
   * @return the stats or HTTP error
   */
  virtual Result<BucketInfo> GetInfo() const noexcept = 0;

  /**
   * @brief Get list of entries in the bucket
   * @return list or HTTP error
   */
  virtual Result<std::vector<EntryInfo>> GetEntryList() const noexcept = 0;

  /**
   * @brief Remove the bucket from server with all the entries
   * @return HTTP or communication error
   */
  virtual Error Remove() const noexcept = 0;

  /**
   * @brief Creates a new bucket
   * @param server_url HTTP url
   * @param name name of the bucket
   * @param options HTTP options
   * @return a pointer to the bucket
   */
  static std::unique_ptr<IBucket> Build(std::string_view server_url, std::string_view name,
                                        const HttpOptions& options) noexcept;
};
}  // namespace reduct

#endif  // REDUCT_CPP_BUCKET_H
