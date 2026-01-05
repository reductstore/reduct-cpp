// Copyright 2026 ReductSoftware UG

#include "reduct/internal/batch_v1.h"

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <chrono>
#include <deque>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

namespace reduct::internal {

int64_t ToMicroseconds(const IBucket::Time& ts) {
  return std::chrono::duration_cast<std::chrono::microseconds>(ts.time_since_epoch()).count();
}

IBucket::Time FromMicroseconds(const std::string& ts) {
  return IBucket::Time() + std::chrono::microseconds(std::stoul(ts));
}

std::string RecordEntry(const IBucket::Batch::Record& record, std::string_view default_entry) {
  return record.entry.empty() ? std::string(default_entry) : record.entry;
}

std::string FormatLabels(const IBucket::LabelMap& labels) {
  std::vector<std::string> formatted;
  formatted.reserve(labels.size());
  for (const auto& [label_key, label_value] : labels) {
    auto value = label_value;
    if (value.find(',') != std::string::npos) {
      value = fmt::format("\"{}\"", value);
    }
    formatted.push_back(fmt::format("{}={}", label_key, value));
  }

  return fmt::format("{}", fmt::join(formatted, ","));
}

std::vector<size_t> SortRecords(const IBucket::Batch& batch, const std::string& default_entry, bool sort_by_entry) {
  std::vector<size_t> order;
  order.reserve(batch.records().size());
  for (size_t i = 0; i < batch.records().size(); ++i) {
    order.push_back(i);
  }

  std::sort(order.begin(), order.end(), [&](auto lhs, auto rhs) {
    const auto& left = batch.records()[lhs];
    const auto& right = batch.records()[rhs];
    if (sort_by_entry) {
      auto left_entry = RecordEntry(left, default_entry);
      auto right_entry = RecordEntry(right, default_entry);
      if (left_entry != right_entry) {
        return left_entry < right_entry;
      }
    }
    if (left.timestamp == right.timestamp) {
      return RecordEntry(left, default_entry) < RecordEntry(right, default_entry);
    }
    return left.timestamp < right.timestamp;
  });

  return order;
}

std::vector<IBucket::ReadableRecord> ParseAndBuildBatchedRecordsV1(
    std::deque<std::optional<std::string>>* data, std::mutex* mutex, bool head, IHttpClient::Headers&& headers) {
  auto parse_csv = [](const std::string& csv) {
    std::vector<std::string> items;
    std::string escaped, item;
    std::stringstream ss(csv);

    while (std::getline(ss, item, ',')) {
      if (item.starts_with("\"") && escaped.empty()) {
        escaped = item.substr(1);
      }

      if (!escaped.empty()) {
        if (item.ends_with("\"")) {
          escaped = escaped.substr(0, escaped.size() - 2);
          items.push_back(escaped);
          escaped = "";
        } else {
          escaped += item;
        }
      } else {
        items.push_back(item);
      }
    }

    auto size = std::stoll(items[0]);
    auto content_type = items[1];
    IBucket::LabelMap labels = {};

    for (auto i = 2; i < items.size(); i++) {
      auto pos = items[i].find('=');
      if (pos == std::string::npos) {
        continue;
      }

      auto key = items[i].substr(0, pos);
      labels[key] = items[i].substr(pos + 1);
    }

    return std::tuple{
        size,
        content_type,
        labels,
    };
  };

  std::vector<IBucket::ReadableRecord> records;
  size_t total_records = std::count_if(
      headers.begin(), headers.end(), [](const auto& header) { return header.first.starts_with("x-reduct-time-"); });

  std::map<std::string, std::string> ordered_headers(headers.begin(), headers.end());
  for (auto header = ordered_headers.begin(); header != ordered_headers.end(); ++header) {
    if (!header->first.starts_with("x-reduct-time-")) {
      continue;
    }

    auto [size, content_type, labels] = parse_csv(header->second);

    IBucket::ReadableRecord record;
    record.timestamp = FromMicroseconds(header->first.substr(14));
    record.size = size;
    record.content_type = content_type;
    record.labels = labels;
    record.Read = [data, mutex, size, head](auto record_callback) {
      if (head) {
        return Error::kOk;
      }

      size_t total = 0;
      while (true) {
        std::optional<std::string> chunk = "";
        {
          std::lock_guard lock(*mutex);
          if (!data->empty()) {
            chunk = std::move(data->front());
            data->pop_front();

            if (chunk->size() > size - total) {
              auto tmp = chunk->substr(0, size - total);
              data->push_front(chunk->substr(size - total));
              chunk = std::move(tmp);
            }
          }
        }

        if (!chunk) {
          break;
        }

        if (chunk->empty()) {
          std::this_thread::sleep_for(std::chrono::microseconds(100));
          continue;
        }

        total += chunk->size();
        if (!record_callback(std::move(*chunk))) {
          break;
        }

        if (total >= size) {
          break;
        }
      }

      return Error::kOk;
    };

    record.last = (records.size() == total_records - 1 && headers["x-reduct-last"] == "true");
    records.push_back(std::move(record));
  }

  return records;
}

Result<IBucket::WriteBatchErrors> ProcessBatchV1(IHttpClient& client, std::string_view bucket_path,
                                                 std::string_view entry_name, IBucket::Batch batch, BatchType type) {
  auto ordered = SortRecords(batch, std::string(entry_name), false);

  std::set<std::string> unique_entries;
  for (auto idx : ordered) {
    unique_entries.insert(RecordEntry(batch.records()[idx], entry_name));
  }

  if (unique_entries.size() > 1) {
    return {
        {},
        Error{.code = -1, .message = "Batch protocol v2 is required to target multiple entries in one request"},
    };
  }

  IHttpClient::Headers headers;
  for (auto idx : ordered) {
    const auto& record = batch.records()[idx];
    const auto key = fmt::format("x-reduct-time-{}", ToMicroseconds(record.timestamp));

    switch (type) {
      case BatchType::kWrite: {
        const auto value = fmt::format("{},{},{}", record.size, record.content_type, FormatLabels(record.labels));
        headers.emplace(key, value);
        break;
      }
      case BatchType::kUpdate: {
        const auto value = fmt::format("0,,{}", FormatLabels(record.labels));
        headers.emplace(key, value);
        break;
      }
      case BatchType::kRemove:
        headers.emplace(key, "0,");
        break;
    }
  }

  Result<std::tuple<std::string, IHttpClient::Headers>> resp_result;
  switch (type) {
    case BatchType::kWrite: {
      const auto content_length = batch.size();
      resp_result = client.Post(fmt::format("{}/{}/batch", bucket_path, entry_name), "application/octet-stream",
                                content_length, std::move(headers),
                                [ordered = std::move(ordered), batch = std::move(batch)](size_t offset, size_t size) {
                                  return std::pair{true, batch.Slice(ordered, offset, size)};
                                });
      break;
    }
    case BatchType::kUpdate:
      resp_result = client.Patch(fmt::format("{}/{}/batch", bucket_path, entry_name), "", std::move(headers));
      break;

    case BatchType::kRemove:
      resp_result = client.Delete(fmt::format("{}/{}/batch", bucket_path, entry_name), std::move(headers));
      break;
  }

  auto [resp, err] = resp_result;
  if (err) {
    return {{}, err};
  }

  IBucket::WriteBatchErrors errors;
  for (const auto& [key, value] : std::get<1>(resp)) {
    if (key.starts_with("x-reduct-error-")) {
      auto pos = value.find(',');
      if (pos == std::string::npos) {
        continue;
      }
      auto status = std::stoi(value.substr(0, pos));
      auto message = value.substr(pos + 1);
      errors.emplace(FromMicroseconds(key.substr(15)), Error{.code = status, .message = message});
    }
  }

  return {errors, Error::kOk};
}

}  // namespace reduct::internal
