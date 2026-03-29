#include "netlib/client.hpp"

namespace netlib
{

TcpClient::TcpClient(asio::io_context& io,
                     std::shared_ptr<ICodec> codec,
                     std::shared_ptr<ILogger> logger,
                     ClientConfig cfg)
    : io_(io), retry_timer_(io), codec_(std::move(codec)), logger_(std::move(logger)), cfg_(cfg)
{
  if (!codec_)
    codec_ = std::make_shared<IdentityCodec>();
  if (!logger_)
    logger_ = std::make_shared<NullLogger>();
}

void TcpClient::connect(asio::ip::tcp::endpoint ep,
                        Session::OnMessage on_msg,
                        Session::OnDisconnect on_disc)
{
  endpoint_ = ep;
  user_on_msg_ = std::move(on_msg);
  user_on_disc_ = std::move(on_disc);
  disconnecting_ = false;
  attempt_ = 0;

  ErrorCode ignored;
  retry_timer_.cancel(ignored);

  do_connect();
}

void TcpClient::disconnect()
{
  disconnecting_ = true;
  ErrorCode ignored;
  retry_timer_.cancel(ignored);
  if (session_)
    session_->close();
}

void TcpClient::do_connect()
{
  if (disconnecting_)
    return;

  ++attempt_;
  auto sock = std::make_shared<asio::ip::tcp::socket>(io_);

  sock->async_connect(endpoint_, [this, sock](const ErrorCode& ec) {
    if (ec) {
      logger_->log(LogLevel::error, ec.message());

      if (cfg_.auto_reconnect && !disconnecting_ &&
          (cfg_.retry.max_attempts == 0 || attempt_ < cfg_.retry.max_attempts)) {
        auto delay = next_delay(cfg_.retry, attempt_);
        retry_timer_.expires_after(delay);
        retry_timer_.async_wait([this](const ErrorCode& tec) {
          if (tec)
            return;
          do_connect();
        });
        return;
      }

      if (user_on_disc_)
        user_on_disc_(session_, ec);
      return;
    }

    auto s = std::make_shared<Session>(std::move(*sock), codec_, logger_, cfg_.session);
    session_ = s;

    s->start(
        [this](std::shared_ptr<Session> ss, ByteVec&& msg) {
          if (user_on_msg_)
            user_on_msg_(std::move(ss), std::move(msg));
        },
        [this](std::shared_ptr<Session> ss, const ErrorCode& dec) {
          on_session_disconnect(std::move(ss), dec);
        });
  });
}

void TcpClient::on_session_disconnect(std::shared_ptr<Session> s, const ErrorCode& ec)
{
  if (user_on_disc_)
    user_on_disc_(s, ec);

  if (disconnecting_)
    return;
  if (!cfg_.auto_reconnect)
    return;
  if (cfg_.retry.max_attempts != 0 && attempt_ >= cfg_.retry.max_attempts)
    return;

  auto delay = next_delay(cfg_.retry, attempt_);
  retry_timer_.expires_after(delay);
  retry_timer_.async_wait([this](const ErrorCode& tec) {
    if (tec)
      return;
    do_connect();
  });
}

} // namespace netlib
