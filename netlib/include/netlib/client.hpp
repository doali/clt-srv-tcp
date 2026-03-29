#pragma once
#include "netlib/asio.hpp"
#include "netlib/retry_policy.hpp"
#include "netlib/session.hpp"

#include <memory>

namespace netlib
{

struct ClientConfig
{
  SessionConfig session;
  bool auto_reconnect = false;
  RetryPolicy retry{};
};

class TcpClient
{
public:
  TcpClient(asio::io_context& io,
            std::shared_ptr<ICodec> codec,
            std::shared_ptr<ILogger> logger,
            ClientConfig cfg);

  void
  connect(asio::ip::tcp::endpoint ep, Session::OnMessage on_msg, Session::OnDisconnect on_disc);
  void disconnect();

  std::shared_ptr<Session> session() const noexcept { return session_; }

private:
  void do_connect();
  void on_session_disconnect(std::shared_ptr<Session> s, const ErrorCode& ec);

  asio::io_context& io_;
  asio::steady_timer retry_timer_;

  asio::ip::tcp::endpoint endpoint_{};
  std::shared_ptr<ICodec> codec_;
  std::shared_ptr<ILogger> logger_;
  ClientConfig cfg_;

  Session::OnMessage user_on_msg_;
  Session::OnDisconnect user_on_disc_;

  std::shared_ptr<Session> session_;
  std::size_t attempt_ = 0;
  bool disconnecting_ = false;
};

} // namespace netlib
