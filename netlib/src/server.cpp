#include "netlib/server.hpp"

#include <stdexcept>

namespace netlib {

TcpServer::TcpServer(asio::io_context& io,
                     asio::ip::tcp::endpoint endpoint,
                     std::shared_ptr<ICodec> codec,
                     std::shared_ptr<ILogger> logger,
                     ServerConfig cfg)
    : io_(io), acceptor_(io), codec_(std::move(codec)), logger_(std::move(logger)), cfg_(cfg)
{
  if (!codec_)
    codec_ = std::make_shared<IdentityCodec>();
  if (!logger_)
    logger_ = std::make_shared<NullLogger>();

  ErrorCode ec;
  acceptor_.open(endpoint.protocol(), ec);
  if (ec)
    throw std::runtime_error(ec.message());

  acceptor_.set_option(asio::socket_base::reuse_address(true), ec);
  if (ec)
    throw std::runtime_error(ec.message());

  acceptor_.bind(endpoint, ec);
  if (ec)
    throw std::runtime_error(ec.message());

  acceptor_.listen(cfg_.backlog, ec);
  if (ec)
    throw std::runtime_error(ec.message());
}

void TcpServer::start(OnConnect on_connect,
                      Session::OnMessage on_msg,
                      Session::OnDisconnect on_disc)
{
  on_connect_ = std::move(on_connect);
  on_msg_ = std::move(on_msg);
  on_disc_ = std::move(on_disc);
  running_ = true;
  do_accept();
}

void TcpServer::stop()
{
  running_ = false;
  ErrorCode ec;
  acceptor_.close(ec);
}

void TcpServer::do_accept()
{
  if (!running_)
    return;

  acceptor_.async_accept([this](const ErrorCode& ec, asio::ip::tcp::socket socket) {
    if (ec) {
      if (running_) {
        logger_->log(LogLevel::error, ec.message());
        do_accept();
      }
      return;
    }

    auto s = std::make_shared<Session>(std::move(socket), codec_, logger_, cfg_.session);
    if (on_connect_)
      on_connect_(s);
    s->start(on_msg_, on_disc_);

    do_accept();
  });
}

} // namespace netlib
