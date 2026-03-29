#pragma once
#include <chrono>
#include <cstddef>

namespace netlib
{

struct RetryPolicy
{
  std::chrono::milliseconds initial_delay{200};
  std::chrono::milliseconds max_delay{5000};
  double backoff = 2.0;
  std::size_t max_attempts = 0; // 0 = infini
};

inline std::chrono::milliseconds next_delay(const RetryPolicy& p, std::size_t attempt)
{
  double d = static_cast<double>(p.initial_delay.count());
  for (std::size_t i = 1; i < attempt; ++i)
    d *= p.backoff;
  auto ms = static_cast<long long>(d);
  if (ms > p.max_delay.count())
    ms = p.max_delay.count();
  if (ms < 0)
    ms = p.max_delay.count();
  return std::chrono::milliseconds(ms);
}

} // namespace netlib
