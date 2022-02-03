// Copyright 2022 Alexey Timin
#ifndef REDUCT_CPP_BUCKET_H
#define REDUCT_CPP_BUCKET_H

#include <chrono>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "reduct/error.h"
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

    /**
     * @brief Serialize to JSON string
     * @return
     */
    [[nodiscard]] std::string ToJsonString() const noexcept;

    /**
     * @brief Parse from JSON string
     * @param json
     * @return
     */
    static Result<Settings> Parse(std::string_view json) noexcept;


    bool operator<=>(const Settings&) const noexcept = default;
    friend std::ostream& operator<<(std::ostream& os, const Settings& settings);
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

  /**
   * @brief Get settings by HTTP request
   * @return
   */
  virtual Result<Settings> GetSettings() const noexcept = 0;

  /**
   * @brief Update settings which has values
   * @param settings
   * @return error
   */
  virtual Error UpdateSettings(const Settings& settings) const noexcept = 0;

  /**
   * @brief Remove the bucket from server with all the entries
   * @return HTTP or communication error
   */
  virtual Error Remove() const noexcept = 0;


  static std::unique_ptr<IBucket> Build(std::string_view server_url, std::string_view name);
};
}  // namespace reduct

#endif  // REDUCT_CPP_BUCKET_H
