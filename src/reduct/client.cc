// Copyright 2022 Alexey Timin


#include "reduct/client.h"

namespace reduct {

std::unique_ptr<IClient> IClient::Build(std::string_view url) { return std::unique_ptr<IClient>(); }
}
