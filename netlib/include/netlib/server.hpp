#pragma once
#include "netlib/asio.hpp"
#include "netlib/session.hpp"

#include <functional>
#include <memory>

namespace netlib
{

struct ServerConfig
{
  SessionConfig session;
  int backlog = asio::socket_base::max_listen_connections;
};

class TcpServer {
public:
  using OnConnect = std::function<void(std::shared_ptr<Session>)>;

  TcpServer(asio::io_context& io,
            asio::ip::tcp::endpoint endpoint,
            std::shared_ptr<ICodec> codec,
            std::shared_ptr<ILogger> logger,
            ServerConfig cfg);

  void start(OnConnect on_connect, Session::OnMessage on_msg, Session::OnDisconnect on_disc);
  void stop();

private:
  void do_accept();

  asio::io_context& io_;
  asio::ip::tcp::acceptor acceptor_;

  std::shared_ptr<ICodec> codec_;
  std::shared_ptr<ILogger> logger_;
  ServerConfig cfg_;

  OnConnect on_connect_;
  Session::OnMessage on_msg_;
  Session::OnDisconnect on_disc_;
  bool running_ = false;
};

} // namespace netlib
