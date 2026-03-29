#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace netlib {

// Framing TCP : [uint32_be length][payload]
class LengthPrefix32Framer {
public:
  std::vector<std::vector<std::byte>> ingest(std::span<const std::byte> chunk);
  static std::vector<std::byte> frame(std::span<const std::byte> payload);

  void reset() { buf_.clear(); expected_.reset(); }
  void set_max_frame_size(std::size_t n) { max_ = n; }

private:
  std::vector<std::byte> buf_;
  std::optional<std::uint32_t> expected_;
  std::size_t max_ = 8u * 1024u * 1024u; // 8 MiB
};

} // namespace netlib
