#pragma once
#include "netlib/asio.hpp"
#include "netlib/codec.hpp"
#include "netlib/framing.hpp"
#include "netlib/logger.hpp"

#include <array>
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <span>

namespace netlib {

struct SessionConfig {
  std::size_t max_write_queue_bytes = 4u * 1024u * 1024u;
  std::chrono::milliseconds read_timeout{0};   // 0 => désactivé
  std::chrono::milliseconds write_timeout{0};  // 0 => désactivé
  std::size_t max_frame_size = 8u * 1024u * 1024u;
};

class Session : public std::enable_shared_from_this<Session> {
public:
  using OnMessage    = std::function<void(std::shared_ptr<Session>, ByteVec&&)>;
  using OnDisconnect = std::function<void(std::shared_ptr<Session>, const ErrorCode&)>;

  Session(asio::ip::tcp::socket socket,
          std::shared_ptr<ICodec> codec,
          std::shared_ptr<ILogger> logger,
          SessionConfig cfg);

  void start(OnMessage on_msg, OnDisconnect on_disc);

  // thread-safe via strand
  void async_send(std::span<const std::byte> msg);
  void close();

  asio::ip::tcp::endpoint remote_endpoint(ErrorCode& ec) const noexcept;

private:
  void do_read();
  void do_write();
  void fail(const ErrorCode& ec);

  void arm_read_timeout();
  void arm_write_timeout();
  void cancel_timers();

  asio::ip::tcp::socket socket_;
  asio::strand<asio::any_io_executor> strand_;
  asio::steady_timer read_timer_;
  asio::steady_timer write_timer_;

  std::shared_ptr<ICodec> codec_;
  std::shared_ptr<ILogger> logger_;
  SessionConfig cfg_;

  LengthPrefix32Framer framer_;
  std::array<std::byte, 8192> read_buf_{};

  std::deque<std::vector<std::byte>> write_q_;
  std::size_t write_q_bytes_ = 0;
  bool writing_ = false;
  bool closed_  = false;

  OnMessage on_msg_;
  OnDisconnect on_disc_;
};

} // namespace netlib
