#pragma once
#include <string_view>

namespace netlib {

enum class LogLevel { trace, debug, info, warn, error };

struct ILogger {
  virtual ~ILogger() = default;
  virtual void log(LogLevel lvl, std::string_view msg) noexcept = 0;
};

struct NullLogger final : ILogger {
  void log(LogLevel, std::string_view) noexcept override {}
};

} // namespace netlib
