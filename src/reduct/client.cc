// Copyright 2022 Alexey Timin

#include "reduct/client.h"

#include <httplib/httplib.h>
#include <nlohmann/json.hpp>

namespace reduct {

class Client : public IClient {
 public:
  explicit Client(std::string_view url) : client_(std::make_unique<httplib::Client>(std::string(url))) {}

  [[nodiscard]] GetInfoResult GetInfo() const override {
    auto res = client_->Get("/info");
    if (res.error() == httplib::Error::Read) {
      return {{}, Error{.code = -1, .message = httplib::to_string(res.error())}};
    }

    nlohmann::json data;
    data = nlohmann::json::parse(res->body, nullptr, false);
    if (data.is_discarded()) {
      return {{}, Error{.code = -1, .message = "Server return malformed request"}};
    }

    if (res->status != 200) {
      return {{}, Error{.code = res->status, .message = data["detail"]}};
    }

    try {
      return {
          ServerInfo{
              .version = data.at("version"),
              .bucket_count = data.at("bucket_count"),
          },
          Error::kOk,
      };
    } catch (const std::exception& e) {
      return {{}, Error{.code = -1, .message = e.what()}};
    }
  }

  [[nodiscard]] std::unique_ptr<IBucket> GetBucket(std::string_view name) const override {
    return std::unique_ptr<IBucket>();
  }
  [[nodiscard]] std::unique_ptr<IBucket> CreateBucket(std::string_view name,
                                                      IBucket::Settings settings) const override {
    return std::unique_ptr<IBucket>();
  }

 private:
  std::unique_ptr<httplib::Client> client_;
};

std::unique_ptr<IClient> IClient::Build(std::string_view url) { return std::make_unique<Client>(url); }
}  // namespace reduct
