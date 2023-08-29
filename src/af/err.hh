#pragma once

#include <string>

namespace af {

struct Error final {
  std::string message;
  Error(std::string &&m) : message(std::move(m)) {}
};

}  // namespace af
