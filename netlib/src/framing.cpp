#include "netlib/framing.hpp"

#include <algorithm>
#include <stdexcept>

namespace netlib
{

static std::uint32_t read_be_u32(const std::byte* p)
{
  auto b0 = static_cast<std::uint32_t>(static_cast<unsigned char>(p[0]));
  auto b1 = static_cast<std::uint32_t>(static_cast<unsigned char>(p[1]));
  auto b2 = static_cast<std::uint32_t>(static_cast<unsigned char>(p[2]));
  auto b3 = static_cast<std::uint32_t>(static_cast<unsigned char>(p[3]));
  return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

static void write_be_u32(std::uint32_t v, std::byte* p)
{
  p[0] = static_cast<std::byte>((v >> 24) & 0xFF);
  p[1] = static_cast<std::byte>((v >> 16) & 0xFF);
  p[2] = static_cast<std::byte>((v >> 8) & 0xFF);
  p[3] = static_cast<std::byte>(v & 0xFF);
}

std::vector<std::byte> LengthPrefix32Framer::frame(std::span<const std::byte> payload)
{
  if (payload.size() > 0xFFFF'FFFFu)
    throw std::length_error("payload too large");
  std::vector<std::byte> out(4 + payload.size());
  write_be_u32(static_cast<std::uint32_t>(payload.size()), out.data());
  std::copy(payload.begin(), payload.end(), out.begin() + 4);
  return out;
}

std::vector<std::vector<std::byte>> LengthPrefix32Framer::ingest(std::span<const std::byte> chunk)
{
  buf_.insert(buf_.end(), chunk.begin(), chunk.end());

  std::vector<std::vector<std::byte>> frames;
  for (;;) {
    if (!expected_) {
      if (buf_.size() < 4)
        break;
      auto len = read_be_u32(buf_.data());
      if (len > max_)
        throw std::length_error("frame exceeds max_frame_size");
      expected_ = len;
      buf_.erase(buf_.begin(), buf_.begin() + 4);
    }

    if (expected_ && buf_.size() >= *expected_) {
      frames.emplace_back(buf_.begin(), buf_.begin() + *expected_);
      buf_.erase(buf_.begin(), buf_.begin() + *expected_);
      expected_.reset();
      continue;
    }
    break;
  }
  return frames;
}

} // namespace netlib
