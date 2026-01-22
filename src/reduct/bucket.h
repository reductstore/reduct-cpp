// Copyright 2022-2026 ReductSoftware UG
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
 * @class IBucket
 * @brief Represents a bucket for storing and retrieving data.
 *
 * A Bucket object allows you to interact with a specific bucket in the storage system.
 * You can perform operations like getting the bucket settings, updating the settings,
 * getting bucket information, retrieving the list of entries, writing records to the bucket,
 * and reading records from the bucket.
 */
class IBucket {
 public:
  virtual ~IBucket() = default;

  enum class QuotaType { kNone, kFifo, kHard };

  /**
   * Status of bucket or entry
   */
  enum class Status { kReady, kDeleting };

  /**se
   * Bucket Settings
   */
  struct Settings {
    std::optional<size_t> max_block_size;
    std::optional<QuotaType> quota_type;
    std::optional<size_t> quota_size;
    std::optional<size_t> max_block_records;

    auto operator<=>(const Settings&) const noexcept = default;
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
    Status status;        // status of bucket (READY or DELETING)

    auto operator<=>(const BucketInfo&) const noexcept = default;
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
    Status status;        // status of entry (READY or DELETING)

    auto operator<=>(const EntryInfo&) const noexcept = default;
    friend std::ostream& operator<<(std::ostream& os, const EntryInfo& info);
  };

  using LabelMap = std::map<std::string, std::string>;

  /**
   * ReadableRecord
   */
  struct ReadableRecord {
    std::string entry;
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
     * Called when HTTP Client is ready to send a chunk with data.
     * @returns pair where first is true to continue sending, false to stop. Second is data chunk to send
     */
    using WriteCallback = std::function<std::pair<bool, std::string>(size_t offset, size_t size)>;

    WriteCallback callback_ = [](auto offset, auto size) { return std::pair{false, ""}; };
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
        return std::pair{true, data.substr(offset, size)};
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
      std::string entry;
      Time timestamp;
      size_t size;
      std::string content_type;
      LabelMap labels;
      std::optional<size_t> data_index;
    };

    /**
     * Add a record to batch
     * @param timestamp
     * @param data
     * @param content_type
     * @param labels
     */
    void AddRecord(Time timestamp, std::string data, std::string content_type = "", LabelMap labels = {}) {
      AddRecord("", timestamp, std::move(data), std::move(content_type), std::move(labels));
    }

    void AddRecord(std::string entry, Time timestamp, std::string data, std::string content_type = "",
                   LabelMap labels = {}) {
      const auto index = body_.size();
      body_.push_back(std::move(data));
      size_ += body_.back().size();
      records_.push_back(
          Record{std::move(entry), timestamp, body_.back().size(), std::move(content_type), std::move(labels), index});
    }

    /**
     * Add an empty record to batch (use for removing)
     * @param timestamp
     */
    void AddRecord(Time timestamp) { records_.push_back(Record{{}, timestamp, 0, "", {}, std::nullopt}); }

    void AddRecord(std::string entry, Time timestamp) {
      records_.push_back(Record{std::move(entry), timestamp, 0, "", {}, std::nullopt});
    }

    void AddOnlyLabels(Time timestamp, LabelMap labels) { AddOnlyLabels("", timestamp, std::move(labels)); }

    void AddOnlyLabels(std::string entry, Time timestamp, LabelMap labels) {
      records_.push_back(Record{std::move(entry), timestamp, 0, "", std::move(labels), std::nullopt});
    }

    [[nodiscard]] const std::vector<Record>& records() const { return records_; }

    [[nodiscard]] std::string Slice(size_t offset, size_t size) const { return Slice(std::nullopt, offset, size); }

    [[nodiscard]] std::string Slice(const std::optional<std::vector<size_t>>& order, size_t offset, size_t size) const {
      if (offset >= size_) {
        return "";
      }

      std::string result;
      const auto process_data = [&](const std::string& data) {
        if (offset < data.size()) {
          auto n = std::min(size, data.size() - offset);
          result.append(data.substr(offset, n));
          size -= n;
          offset = 0;
        } else {
          offset -= data.size();
        }
      };

      if (order) {
        for (auto idx : *order) {
          const auto& record = records_.at(idx);
          if (!record.data_index) {
            continue;
          }
          process_data(body_.at(*record.data_index));
          if (size == 0) {
            break;
          }
        }
      } else {
        for (const auto& data : body_) {
          process_data(data);
          if (size == 0) {
            break;
          }
        }
      }

      return result;
    }

    [[nodiscard]] uint64_t size() const { return size_; }

   private:
    std::vector<Record> records_;
    std::vector<std::string> body_;
    uint64_t size_ = 0;
  };

  /**
   * @deprecated Use BatchErrors instead
   */
  using WriteBatchCallback = std::function<void(Batch*)>;
  using BatchCallback = std::function<void(Batch*)>;

  /**
   * @deprecated Use BatchErrors instead
   */
  using BatchErrors = std::map<Time, Error>;
  using BatchRecordErrors = std::map<std::string, BatchErrors>;

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
   * @param ts timestamp, if it is nullopt, the method will use current time
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
  [[nodiscard]] virtual Result<BatchErrors> WriteBatch(std::string_view entry_name,
                                                       BatchCallback callback) const noexcept = 0;

  /**
   * Write a batch of records in one HTTP request (targeting multiple entries)
   * @param callback a callback to add records to batch
   * @return HTTP error or map of errors for each record
   */
  [[nodiscard]] virtual Result<BatchRecordErrors> WriteBatch(BatchCallback callback) const noexcept = 0;

  /**
   * Write labels of an existing record
   *
   * Provide a label with empty value to remove it
   *
   * @param entry_name entry in bucket
   * @param options options with timestamp, labels (content type is ignored)
   * @return HTTP or communication error
   */
  virtual Error Update(std::string_view entry_name, const WriteOptions& options) const noexcept = 0;

  /**
   * Update labels of an existing record in a batch

   * @param entry_name entry in bucket
   * @param callback a callback to add records to batch
   * @return HTTP error or map of errors for each record
   */
  [[nodiscard]] virtual Result<BatchErrors> UpdateBatch(std::string_view entry_name,
                                                        BatchCallback callback) const noexcept = 0;

  /**
   * Update labels of existing records in a batch (targeting multiple entries)
   * @param callback a callback to add records to batch
   * @return HTTP error or map of errors for each record
   */
  [[nodiscard]] virtual Result<BatchRecordErrors> UpdateBatch(BatchCallback callback) const noexcept = 0;

  /**
   * Query options
   */
  struct QueryOptions {
    std::optional<std::string> when;  ///< query condition
    std::optional<bool> strict;       ///< strict mode
    std::optional<std::string> ext;   ///< additional parameters for extensions
    std::optional<std::chrono::milliseconds> ttl;  ///< time to live
    bool continuous = false;  ///< continuous query. If true,
                              /// the method returns the latest record and waits for the next one
    std::chrono::milliseconds poll_interval = std::chrono::milliseconds(1000);  ///< poll interval for continuous query
    bool head_only = false;                                                     ///< read only metadata
  };

  /**
   * @brief Query data for a time interval
   * @param entry_name
   * @param start start time point ,if nullopt then from very beginning
   * @param stop stop time point, if nullopt then until the last record
   * @param options
   * @param callback return  next record If you want to stop querying, return false
   * @return
   */
  [[nodiscard]] virtual Error Query(std::string_view entry_name, std::optional<Time> start, std::optional<Time> stop,
                                    QueryOptions options, ReadRecordCallback callback) const noexcept = 0;

  /**
   * @brief  Query data for multiple entries for a time interval
   * @param entry_names
   * @param start start time point ,if nullopt then from very beginning
   * @param stop stop time point, if nullopt then until the last record
   * @param options
   * @param callback return  next record If you want to stop querying, return false
   * @return
   */
  [[nodiscard]] virtual Error Query(const std::vector<std::string>& entry_names, std::optional<Time> start,
                                    std::optional<Time> stop, QueryOptions options,
                                    ReadRecordCallback callback) const noexcept = 0;
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
   * @brief Remove a record from the entry
   * @param entry_name
   * @param timestamp
   * @return HTTP or communication error
   */
  virtual Error RemoveRecord(std::string_view entry_name, Time timestamp) const noexcept = 0;

  /**
   * @brief Remove a batch of records from the entry
   * @param entry_name
   * @param callback a callback to add records to batch
   * @return HTTP error or map of errors for each record
   */
  virtual Result<BatchErrors> RemoveBatch(std::string_view entry_name, BatchCallback callback) const noexcept = 0;

  /**
   * @brief Remove a batch of records from multiple entries
   * @param callback a callback to add records to batch
   * @return HTTP error or map of errors for each record
   */
  virtual Result<BatchRecordErrors> RemoveBatch(BatchCallback callback) const noexcept = 0;

  /**
   * @brief Remove records of an entry by query
   * @param entry_name
   * @param start start time point
   * @param stop stop time point
   * @param options query options. TTL, continuous, poll_interval, head_only are ignored
   * @return HTTP  error or number of removed records
   */
  virtual Result<uint64_t> RemoveQuery(std::string_view entry_name, std::optional<Time> start, std::optional<Time> stop,
                                       QueryOptions options) const noexcept = 0;

  /**
   * @brief Remove records of multiple entries by query
   * @param entries vector of entry names
   * @param start start time point
   * @param stop stop time point
   * @param options query options. TTL, continuous, poll_interval, head_only are ignored
   * @return HTTP  error or number of removed records
   */
  virtual Result<uint64_t> RemoveQuery(std::vector<std::string> entries, std::optional<Time> start,
                                       std::optional<Time> stop, QueryOptions options) const noexcept = 0;

  /**
   * @brief Rename an entry
   * @param old_name entry name to rename
   * @param new_name
   * @return
   */
  virtual Error RenameEntry(std::string_view old_name, std::string_view new_name) const noexcept = 0;

  /**
   * @brief Rename the bucket
   * @param new_name
   * @return
   */
  virtual Error Rename(std::string_view new_name) noexcept = 0;

  struct QueryLinkOptions {
    std::optional<Time> start;
    std::optional<Time> stop;
    QueryOptions query_options = {};
    uint64_t record_index = 0;  // index of record in the query result to return
    std::optional<Time> expire_at = std::nullopt;
    std::optional<std::string> file_name = std::nullopt;
    std::optional<std::string> base_url = std::nullopt;
  };

  /**
   * @brief  Creates a query link for accessing data
   * @param entry_name entry in the bucket
   * @param options options for the query link
   * @return a query link or an error
   */
  virtual Result<std::string> CreateQueryLink(std::string_view entry_name, QueryLinkOptions options) const noexcept = 0;

  /**
   * @brief  Creates a query link for accessing data
   *
   * You can specify multiple entries in the bucket
   *
   * @param entries list of entries in the bucket
   * @param options options for the query link
   * @return a query link or an error
   */
  virtual Result<std::string> CreateQueryLink(const std::vector<std::string>& entries,
                                              QueryLinkOptions options) const noexcept = 0;

  /**
   * @brief Creates a new bucket
   * @param server_url HTTP url
   * @param name name of the bucket
   * @param options HTTP options
   * @return a pointer to the bucket
   */
  static std::unique_ptr<IBucket> Build(std::string_view server_url, std::string_view name,
                                        const HttpOptions& options) noexcept;
  static std::unique_ptr<IBucket> Build(std::string_view server_url, std::string_view name, const HttpOptions& options,
                                        std::optional<std::string> api_version) noexcept;
};
}  // namespace reduct

#endif  // REDUCT_CPP_BUCKET_H
