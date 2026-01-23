// Copyright 2026 ReductSoftware UG

#include "reduct/internal/batch_v2.h"

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <deque>
#include <limits>
#include <mutex>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "reduct/internal/headers.h"

namespace reduct::internal {

struct RecordHeaderV2 {
  uint64_t content_length;
  std::string content_type;
  IBucket::LabelMap labels;
};

std::string EncodeEntryName(std::string_view entry) {
  std::string encoded;
  encoded.reserve(entry.size());
  for (const auto ch : entry) {
    if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '!' || ch == '#' || ch == '$' || ch == '%' || ch == '&' ||
        ch == '\'' || ch == '*' || ch == '+' || ch == '-' || ch == '.' || ch == '^' || ch == '_' || ch == '`' ||
        ch == '|' || ch == '~') {
      encoded.push_back(ch);
    } else {
      encoded.append(fmt::format("%{:02X}", static_cast<unsigned char>(ch)));
    }
  }
  return encoded;
}

static std::string Trim(std::string_view value) {
  auto start = value.find_first_not_of(" \t");
  if (start == std::string_view::npos) {
    return "";
  }

  auto end = value.find_last_not_of(" \t");
  return std::string(value.substr(start, end - start + 1));
}

static std::optional<std::string> DecodeEntryName(std::string_view encoded) {
  std::string decoded;
  decoded.reserve(encoded.size());
  for (size_t i = 0; i < encoded.size(); ++i) {
    if (encoded[i] == '%') {
      if (i + 2 >= encoded.size()) {
        return std::nullopt;
      }

      auto hex = encoded.substr(i + 1, 2);
      try {
        auto value = static_cast<char>(std::stoi(std::string(hex), nullptr, 16));
        decoded.push_back(value);
        i += 2;
      } catch (...) {
        return std::nullopt;
      }
    } else {
      decoded.push_back(encoded[i]);
    }
  }

  return decoded;
}

static std::vector<std::string> ParseEncodedList(std::string_view raw) {
  std::vector<std::string> items;
  std::stringstream ss{std::string(raw)};
  std::string item;
  while (std::getline(ss, item, ',')) {
    item = Trim(item);
    if (!item.empty()) {
      if (auto decoded = DecodeEntryName(item)) {
        items.push_back(std::move(*decoded));
      }
    }
  }
  return items;
}

static std::optional<std::pair<size_t, uint64_t>> ParseBatchedHeaderNameV2(const std::string& name) {
  if (!name.starts_with(kHeaderPrefix) || name.starts_with(kHeaderErrorPrefix)) {
    return std::nullopt;
  }

  auto suffix = name.substr(kHeaderPrefix.size());
  auto dash = suffix.rfind('-');
  if (dash == std::string::npos) {
    return std::nullopt;
  }

  auto entry_str = suffix.substr(0, dash);
  auto delta_str = suffix.substr(dash + 1);

  try {
    auto entry_idx = static_cast<size_t>(std::stoul(entry_str));
    auto delta = std::stoull(delta_str);
    return std::pair{entry_idx, delta};
  } catch (...) {
    return std::nullopt;
  }
}

static std::vector<std::pair<std::string, std::optional<std::string>>> ParseLabelDeltaOps(
    std::string_view raw, const std::optional<std::vector<std::string>>& label_names) {
  std::vector<std::pair<std::string, std::optional<std::string>>> ops;
  size_t pos = 0;
  while (pos < raw.size()) {
    auto eq = raw.find('=', pos);
    if (eq == std::string::npos) {
      break;
    }

    auto raw_key = Trim(raw.substr(pos, eq - pos));
    std::string key = raw_key;
    if (label_names) {
      bool digits = !key.empty() && std::all_of(key.begin(), key.end(),
                                                [](char ch) { return std::isdigit(static_cast<unsigned char>(ch)); });
      if (digits) {
        auto idx = std::stoul(key);
        if (idx < label_names->size()) {
          key = label_names->at(idx);
        }
      }
    }

    pos = eq + 1;
    std::optional<std::string> value;

    if (pos < raw.size() && raw[pos] == '"') {
      ++pos;
      auto quote = raw.find('"', pos);
      if (quote == std::string::npos) {
        break;
      }
      value = std::string(raw.substr(pos, quote - pos));
      pos = quote + 1;
    } else {
      auto comma = raw.find(',', pos);
      auto slice = raw.substr(pos, comma == std::string::npos ? raw.size() - pos : comma - pos);
      auto trimmed_value = Trim(slice);
      if (!trimmed_value.empty()) {
        value = std::move(trimmed_value);
      }
      pos = comma == std::string::npos ? raw.size() : comma;
    }

    ops.emplace_back(std::move(key), std::move(value));
    if (pos < raw.size() && raw[pos] == ',') {
      ++pos;
    }
    while (pos < raw.size() && std::isspace(static_cast<unsigned char>(raw[pos]))) {
      ++pos;
    }
  }

  return ops;
}

static IBucket::LabelMap ApplyLabelDelta(std::string_view raw, const IBucket::LabelMap& base,
                                         const std::optional<std::vector<std::string>>& label_names) {
  IBucket::LabelMap labels = base;
  for (auto& [key, value] : ParseLabelDeltaOps(raw, label_names)) {
    if (value) {
      labels[key] = *value;
    } else {
      labels.erase(key);
    }
  }
  return labels;
}

static std::optional<RecordHeaderV2> ParseRecordHeaderV2(const std::string& raw,
                                                         const std::optional<RecordHeaderV2>& previous,
                                                         const std::optional<std::vector<std::string>>& label_names) {
  auto first_comma = raw.find(',');
  auto length_str =
      Trim(first_comma == std::string::npos ? std::string_view(raw) : std::string_view(raw).substr(0, first_comma));
  if (length_str.empty()) {
    return std::nullopt;
  }

  uint64_t content_length = 0;
  try {
    content_length = std::stoull(length_str);
  } catch (...) {
    return std::nullopt;
  }

  std::string content_type;
  std::optional<std::string> labels_raw;
  if (first_comma != std::string::npos) {
    auto rest = std::string_view(raw).substr(first_comma + 1);
    auto second_comma = rest.find(',');
    if (second_comma == std::string::npos) {
      content_type = Trim(rest);
    } else {
      content_type = Trim(rest.substr(0, second_comma));
      labels_raw = std::string(rest.substr(second_comma + 1));
    }
  }

  if (content_type.empty()) {
    if (previous) {
      content_type = previous->content_type;
    } else {
      content_type = "application/octet-stream";
    }
  }

  auto labels = previous ? previous->labels : IBucket::LabelMap{};
  if (labels_raw) {
    labels = ApplyLabelDelta(*labels_raw, labels, label_names);
  }

  return RecordHeaderV2{content_length, std::move(content_type), std::move(labels)};
}

std::vector<IBucket::ReadableRecord> ParseAndBuildBatchedRecordsV2(std::deque<std::optional<std::string>>* data,
                                                                   std::mutex* mutex, bool head,
                                                                   IHttpClient::Headers&& headers) {
  std::vector<IBucket::ReadableRecord> records;
  auto entries_it = headers.find(std::string(kHeaderEntries));
  auto start_ts_it = headers.find(std::string(kHeaderStartTs));
  if (entries_it == headers.end() || start_ts_it == headers.end()) {
    return records;
  }

  auto entries = ParseEncodedList(entries_it->second);
  if (entries.empty()) {
    return records;
  }

  uint64_t start_ts = 0;
  try {
    start_ts = std::stoull(start_ts_it->second);
  } catch (...) {
    return records;
  }

  std::optional<std::vector<std::string>> label_names;
  if (auto labels_it = headers.find(std::string(kHeaderLabels)); labels_it != headers.end()) {
    label_names = ParseEncodedList(labels_it->second);
  }

  std::vector<std::tuple<size_t, uint64_t, std::string>> parsed_headers;
  parsed_headers.reserve(headers.size());
  for (const auto& [key, value] : headers) {
    auto parsed = ParseBatchedHeaderNameV2(key);
    if (!parsed) {
      continue;
    }
    parsed_headers.emplace_back(parsed->first, parsed->second, value);
  }

  std::sort(parsed_headers.begin(), parsed_headers.end(), [](const auto& lhs, const auto& rhs) {
    if (std::get<0>(lhs) == std::get<0>(rhs)) {
      return std::get<1>(lhs) < std::get<1>(rhs);
    }
    return std::get<0>(lhs) < std::get<0>(rhs);
  });

  std::vector<std::optional<RecordHeaderV2>> last_header(entries.size());
  for (const auto& [entry_idx, delta, value] : parsed_headers) {
    if (entry_idx >= entries.size()) {
      continue;
    }

    auto header = ParseRecordHeaderV2(value, last_header[entry_idx], label_names);
    if (!header) {
      continue;
    }

    last_header[entry_idx] = header;

    IBucket::ReadableRecord record;
    record.entry = entries[entry_idx];
    record.timestamp = IBucket::Time() + std::chrono::microseconds(start_ts + delta);
    record.size = header->content_length;
    record.content_type = header->content_type;
    record.labels = header->labels;
    record.last = false;
    record.Read = [data, mutex, head, size = header->content_length](auto record_callback) {
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

            if (chunk && chunk->size() > size - total) {
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

    records.push_back(std::move(record));
  }

  if (!records.empty() && headers[std::string(kHeaderLast)] == "true") {
    records.back().last = true;
  }

  return records;
}

static std::string BuildHeaderValueV2(const IBucket::Batch::Record& record, BatchType type) {
  switch (type) {
    case BatchType::kRemove:
      return "";
    case BatchType::kUpdate: {
      auto labels = FormatLabels(record.labels);
      if (labels.empty()) {
        return "0";
      }
      return fmt::format("0,,{}", labels);
    }
    case BatchType::kWrite: {
      auto labels = FormatLabels(record.labels);
      if (labels.empty()) {
        return fmt::format("{},{}", record.size, record.content_type);
      }
      return fmt::format("{},{},{}", record.size, record.content_type, labels);
    }
  }
  return "";
}

struct BatchV2Request {
  std::vector<size_t> ordered;
  std::vector<std::string> entries;
  uint64_t start_ts = 0;
  IHttpClient::Headers headers;
};

Result<BatchV2Request> BuildBatchV2Request(std::string_view default_entry, const IBucket::Batch& batch, BatchType type,
                                           bool require_entry) {
  BatchV2Request request;
  request.ordered = SortRecords(batch, std::string(default_entry), true);
  if (request.ordered.empty()) {
    return {request, Error::kOk};
  }

  request.start_ts = std::numeric_limits<uint64_t>::max();
  std::unordered_map<std::string, size_t> entry_indices;
  for (auto idx : request.ordered) {
    const auto& record = batch.records()[idx];
    auto entry = RecordEntry(record, default_entry);
    if (require_entry && entry.empty()) {
      return {BatchV2Request{}, Error{.code = 400, .message = "Entry name is required"}};
    }
    auto ts = static_cast<uint64_t>(ToMicroseconds(record.timestamp));
    request.start_ts = std::min(request.start_ts, ts);
    if (!entry_indices.contains(entry)) {
      entry_indices[entry] = request.entries.size();
      request.entries.push_back(entry);
    }
  }

  std::vector<std::string> encoded_entries;
  encoded_entries.reserve(request.entries.size());
  for (const auto& entry : request.entries) {
    encoded_entries.push_back(EncodeEntryName(entry));
  }

  request.headers.emplace(std::string(kHeaderEntries), fmt::format("{}", fmt::join(encoded_entries, ",")));
  request.headers.emplace(std::string(kHeaderStartTs), std::to_string(request.start_ts));

  for (auto idx : request.ordered) {
    const auto& record = batch.records()[idx];
    const auto entry = RecordEntry(record, default_entry);
    const auto entry_idx = entry_indices[entry];
    const auto delta = static_cast<uint64_t>(ToMicroseconds(record.timestamp)) - request.start_ts;

    auto key = fmt::format("{}{}-{}", kHeaderPrefix, entry_idx, delta);
    request.headers.emplace(std::move(key), BuildHeaderValueV2(record, type));
  }

  return {request, Error::kOk};
}

Result<std::tuple<std::string, IHttpClient::Headers>> SendBatchV2(IHttpClient* client, std::string_view io_path,
                                                                  BatchType type, IBucket::Batch batch,
                                                                  std::vector<size_t> ordered,
                                                                  IHttpClient::Headers headers) {
  switch (type) {
    case BatchType::kWrite: {
      const auto content_length = batch.size();
      return client->Post(fmt::format("{}/write", io_path), "application/octet-stream", content_length,
                          std::move(headers),
                          [ordered = std::move(ordered), batch = std::move(batch)](size_t offset, size_t size) {
                            return std::pair{true, batch.Slice(ordered, offset, size)};
                          });
    }
    case BatchType::kUpdate:
      return client->Patch(fmt::format("{}/update", io_path), "", std::move(headers));
    case BatchType::kRemove:
      return client->Delete(fmt::format("{}/remove", io_path), std::move(headers));
  }

  return {{}, Error{.code = -1, .message = "Unsupported batch type"}};
}

Result<IBucket::BatchErrors> ProcessBatchV2(IHttpClient* client, std::string_view io_path, std::string_view entry_name,
                                            IBucket::Batch batch, BatchType type) {
  auto [request, request_err] = BuildBatchV2Request(entry_name, batch, type, false);
  if (request_err) {
    return {{}, std::move(request_err)};
  }
  if (request.ordered.empty()) {
    return {IBucket::BatchErrors{}, Error::kOk};
  }

  auto [resp, err] =
      SendBatchV2(client, io_path, type, std::move(batch), std::move(request.ordered), std::move(request.headers));
  if (err) {
    return {{}, err};
  }

  IBucket::BatchErrors errors;
  for (const auto& [key, value] : std::get<1>(resp)) {
    if (key.starts_with(kHeaderErrorPrefix)) {
      auto prefix = key.substr(kHeaderErrorPrefix.size());
      auto dash = prefix.rfind('-');
      if (dash == std::string::npos) {
        continue;
      }
      auto delta = std::stoull(prefix.substr(dash + 1));
      auto pos = value.find(',');
      if (pos == std::string::npos) {
        continue;
      }
      auto status = std::stoi(value.substr(0, pos));
      auto message = value.substr(pos + 1);
      errors.emplace(FromMicroseconds(std::to_string(request.start_ts + delta)),
                     Error{.code = status, .message = message});
    }
  }

  return {errors, Error::kOk};
}

Result<IBucket::BatchRecordErrors> ProcessBatchV2Records(IHttpClient* client, std::string_view io_path,
                                                         IBucket::Batch batch, BatchType type) {
  auto [request, request_err] = BuildBatchV2Request("", batch, type, true);
  if (request_err) {
    return {{}, std::move(request_err)};
  }
  if (request.ordered.empty()) {
    return {IBucket::BatchRecordErrors{}, Error::kOk};
  }

  auto [resp, err] =
      SendBatchV2(client, io_path, type, std::move(batch), std::move(request.ordered), std::move(request.headers));
  if (err) {
    return {{}, err};
  }

  const auto& resp_headers = std::get<1>(resp);
  auto entries_it = resp_headers.find(std::string(kHeaderEntries));
  auto start_ts_it = resp_headers.find(std::string(kHeaderStartTs));

  std::vector<std::string> entries;
  if (entries_it != resp_headers.end()) {
    entries = ParseEncodedList(entries_it->second);
  }
  if (entries.empty()) {
    entries = request.entries;
  }

  uint64_t start_ts = request.start_ts;
  if (start_ts_it != resp_headers.end()) {
    try {
      start_ts = std::stoull(start_ts_it->second);
    } catch (...) {
      start_ts = request.start_ts;
    }
  }

  IBucket::BatchRecordErrors errors;
  for (const auto& [key, value] : std::get<1>(resp)) {
    if (key.starts_with(kHeaderErrorPrefix)) {
      auto prefix = key.substr(kHeaderErrorPrefix.size());
      auto dash = prefix.rfind('-');
      if (dash == std::string::npos) {
        continue;
      }
      auto entry_idx = std::stoul(prefix.substr(0, dash));
      if (entry_idx >= entries.size()) {
        continue;
      }
      auto delta = std::stoull(prefix.substr(dash + 1));
      auto pos = value.find(',');
      if (pos == std::string::npos) {
        continue;
      }
      auto status = std::stoi(value.substr(0, pos));
      auto message = value.substr(pos + 1);
      const auto& entry = entries[entry_idx];
      errors[entry].emplace(FromMicroseconds(std::to_string(start_ts + delta)),
                            Error{.code = status, .message = message});
    }
  }

  return {errors, Error::kOk};
}

}  // namespace reduct::internal
