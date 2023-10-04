// Copyright 2022-2023 Alexey Timin
#ifndef REDUCT_CPP_BUCKET_H
#define REDUCT_CPP_BUCKET_H

#include <chrono>
#include <functional>
#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <utility>
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
  virtual ~IBucket() = default;

  enum class QuotaType { kNone, kFifo };

  /**
   * Bucket Settings
   */
  struct Settings {
    std::optional<size_t> max_block_size;
    std::optional<QuotaType> quota_type;
    std::optional<size_t> quota_size;
    std::optional<size_t> max_block_records;

    bool operator<=>(const Settings&) const noexcept = default;
    friend std::ostream& operator<<(std::ostream& os, const Settings& settings);
  };

  using Time = std::chrono::time_point<std::chrono::system_clock>;

  /**
   * Stats of bucket
   */
  struct BucketInfo {
    std::string name;     // name of bucket
    size_t entry_count;   // number of entries in the bucket
    size_t size;          // size of stored data in the bucket in bytes
    Time oldest_record;   // timestamp of the oldest record in the bucket
    Time latest_record;   // timestamp of the latest record in the bucket
    bool is_provisioned;  // is bucket provisioned, you can't remove it or change settings

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

  using LabelMap = std::map<std::string, std::string>;

  /**
   * ReadableRecord
   */
  struct ReadableRecord {
    Time timestamp;
    size_t size;
    bool last;
    LabelMap labels;
    std::string content_type;

    /**
     * Called when HTTP Client received a chunk with data. It may return false to stop transferring
     */
    using ReadCallback = std::function<bool(std::string_view)>;

    /**
     * Function to receive data in chunks
     */
    std::function<Error(ReadCallback)> Read;

    /**
     * Read all data
     * @return received string
     */
    Result<std::string> ReadAll() const {
      std::string data;
      auto err = Read([&data](auto chunk) {
        data.append(chunk);
        return true;
      });

      return {std::move(data), std::move(err)};
    }
  };

  /**
   * Callback to receive a readable record
   */
  using ReadRecordCallback = std::function<bool(const ReadableRecord& record)>;

  /**
   * WritableRecord
   */
  struct WritableRecord {
    /**
     * A write ccallbackallback is called when HTTP Client is ready to send a chunk with data.
     * @returns last flag and data to write
     */
    using WriteCallback = std::function<std::pair<bool, std::string>(size_t offset, size_t size)>;

    WriteCallback callback_ = [](auto offset, auto size) { return std::pair{true, ""}; };
    size_t content_length_;

    /**
     * Receives write callback and content length to pass it to HTTP client
     * @param content_length
     * @param cb
     */
    void Write(size_t content_length, WriteCallback&& cb) {
      content_length_ = content_length;
      callback_ = std::move(cb);
    }

    /**
     * Sends the whole blob to write
     * @param data
     */
    void WriteAll(std::string data) {
      content_length_ = data.size();
      callback_ = [data = std::move(data)](size_t offset, size_t size) {
        return std::pair{data.size() <= offset + size, data.substr(offset, size)};
      };
    }
  };

  using WriteRecordCallback = std::function<void(WritableRecord*)>;

  /**
   * Batch of records
   */
  class Batch {
   public:
    struct Record {
      Time timestamp;
      size_t size;
      std::string content_type;
      LabelMap labels;
    };

    /**
     * Add a record to batch
     * @param timestamp
     * @param data
     * @param content_type
     * @param labels
     */
    void AddRecord(Time timestamp, const std::string& data, std::string content_type = "application/octet-stream",
                   LabelMap labels = {}) {
      records_[timestamp] = Record{timestamp, data.size(), std::move(content_type), std::move(labels)};
      body_ += data;
    }

    [[nodiscard]] const std::map<Time, Record>& records() const { return records_; }
    [[nodiscard]] const std::string& body() const { return body_; }

   private:
    std::map<Time, Record> records_;
    std::string body_;
  };

  using WriteBatchCallback = std::function<void(Batch*)>;
  using WriteBatchErrors = std::map<Time, Error>;

  /**
   * Read a record in chunks
   * @param entry_name entry in bucket
   * @param ts timestamp, if it is nullopt, the method returns the latest record
   * @param callback called with callback. To continue receiving it should return true
   * @return HTTP or communication error
   */
  virtual Error Read(std::string_view entry_name, std::optional<Time> ts,
                     ReadRecordCallback callback) const noexcept = 0;

  /**
   * Read only metadata of a record
   * @param entry_name entry in bucket
   * @param ts timestamp, if it is nullopt, the method returns the latest record
   * @param callback called with callback. To continue receiving it should return true
   * @return HTTP or communication error
   */
  virtual Error Head(std::string_view entry_name, std::optional<Time> ts,
                     ReadRecordCallback callback) const noexcept = 0;
  /**
   * Write a record
   * @param entry_name entry in bucket
   * @param ts timestamp, if it is nullopt, the method returns the latest record
   * @param callback
   * @return HTTP or communication error
   */
  virtual Error Write(std::string_view entry_name, std::optional<Time> ts,
                      WriteRecordCallback callback) const noexcept = 0;

  /**
   * Options for writing a record
   */
  struct WriteOptions {
    std::optional<Time> timestamp;
    LabelMap labels;
    std::string content_type;
  };

  /**
   * Write a record (extended version)
   * @param entry_name entry in bucket
   * @param options options with timestamp, labels and content type
   * @param callback
   * @return HTTP or communication error
   */
  virtual Error Write(std::string_view entry_name, const WriteOptions& options,
                      WriteRecordCallback callback) const noexcept = 0;

  /**
   * Write a batch of records in one HTTP request
   * @param entry_name entry in bucket
   * @param callback a callback to add records to batch
   * @return HTTP error or map of errors for each record
   */
  [[nodiscard]] virtual Result<WriteBatchErrors> WriteBatch(std::string_view entry_name,
                           WriteBatchCallback callback) const noexcept = 0;

  /**
   * Query options
   */
  struct QueryOptions {
    std::optional<std::chrono::milliseconds> ttl;  ///< time to live
    LabelMap include;                              ///< include labels
    LabelMap exclude;                              ///< exclude labels
    bool continuous;  ///< continuous query. If true, the method returns the latest record and waits for the next one
    std::chrono::milliseconds poll_interval;  ///< poll interval for continuous query
    bool head_only;                           ///< read only metadata
    std::optional<size_t> limit;              ///< limit number of records
  };

  /**
   * Query data for a time interval
   * @param entry_name
   * @param start start time point ,if nullopt then from very beginning
   * @param stop stop time point, if nullopt then until the last record
   * @param options
   * @param callback return  next record If you want to stop querying, return false
   * @return
   */
  [[nodiscard]] virtual Error Query(
      std::string_view entry_name, std::optional<Time> start, std::optional<Time> stop, QueryOptions options,
      ReadRecordCallback callback = [](auto) { return false; }) const noexcept = 0;
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
   * @brief Remove an entry from the bucket
   * @param entry_name
   * @return HTTP or communication error
   */
  virtual Error RemoveEntry(std::string_view entry_name) const noexcept = 0;

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
