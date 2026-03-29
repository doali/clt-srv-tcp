#pragma once
#include <cstddef>
#include <span>
#include <vector>

namespace netlib {

using ByteVec = std::vector<std::byte>;

struct ICodec {
  virtual ~ICodec() = default;
  virtual ByteVec encode(std::span<const std::byte> msg) = 0;
  virtual ByteVec decode(std::span<const std::byte> frame) = 0;
};

// codec identité (binaire)
struct IdentityCodec final : ICodec {
  ByteVec encode(std::span<const std::byte> msg) override {
    return ByteVec(msg.begin(), msg.end());
  }
  ByteVec decode(std::span<const std::byte> frame) override {
    return ByteVec(frame.begin(), frame.end());
  }
};

} // namespace netlib
