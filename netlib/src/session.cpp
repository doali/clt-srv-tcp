#include "netlib/session.hpp"












#include <boost/system/error_code.hpp>

namespace netlib {

static ErrorCode ec_errc(boost::system::errc::errc_t e)




{
  // Boost.System fournit errc::make_error_code. [5](https://github.com/cplusplus/networking-ts)
  return boost::system::errc::make_error_code(e);
}

    Session::Session(asio::ip::tcp::socket socket,
                 std::shared_ptr<ICodec> codec,
                 std::shared_ptr<ILogger> logger,
                 SessionConfig cfg)
    : socket_(std::move(socket)), strand_(asio::make_strand(socket_.get_executor())),
      read_timer_(socket_.get_executor()), write_timer_(socket_.get_executor()),
      codec_(std::move(codec)), logger_(std::move(logger)), cfg_(cfg)
{
  framer_.set_max_frame_size(cfg_.max_frame_size);
  if (!codec_)
    codec_ = std::make_shared<IdentityCodec>();
  if (!logger_)
    logger_ = std::make_shared<NullLogger>();
}

void Session::start(OnMessage on_msg, OnDisconnect on_disc)
{
  asio::dispatch(strand_,
                 [self = shared_from_this(),
                  on_msg = std::move(on_msg),
                  on_disc = std::move(on_disc)]() mutable {
                   self->on_msg_ = std::move(on_msg);
                   self->on_disc_ = std::move(on_disc);
                   self->do_read();
                 });
}

void Session::async_send(std::span<const std::byte> msg)
{
  auto bytes = codec_->encode(msg);
  auto framed = LengthPrefix32Framer::frame(bytes);

  asio::post(strand_, [self = shared_from_this(), framed = std::move(framed)]() mutable {
    if (self->closed_)
      return;

    const auto add = framed.size();
    if (self->write_q_bytes_ + add > self->cfg_.max_write_queue_bytes) {
      self->logger_->log(LogLevel::warn, "write queue overflow; closing session");
      self->fail(ec_errc(boost::system::errc::no_buffer_space));
      return;
    }

    self->write_q_bytes_ += add;
    self->write_q_.push_back(std::move(framed));

    if (!self->writing_)
      self->do_write();
  });
}

void Session::close()
{
  asio::post(strand_, [self = shared_from_this()] {
    self->fail(ec_errc(boost::system::errc::operation_canceled));
  });
}

asio::ip::tcp::endpoint Session::remote_endpoint(ErrorCode& ec) const noexcept
{
  return socket_.remote_endpoint(ec);
}

void Session::do_read()
{
  if (closed_)
    return;

  arm_read_timeout();

  socket_.async_read_some(
      asio::buffer(read_buf_.data(), read_buf_.size()),
      asio::bind_executor(strand_, [self = shared_from_this()](const ErrorCode& ec, std::size_t n) {
        if (ec) {
          self->fail(ec);
          return;
        }

        ErrorCode ignored;
        self->read_timer_.cancel(ignored);

        try {
          auto frames = self->framer_.ingest(std::span<const std::byte>(self->read_buf_.data(), n));
          for (auto& f : frames) {
            auto msg = self->codec_->decode(std::span<const std::byte>(f.data(), f.size()));
            if (self->on_msg_)
              self->on_msg_(self, std::move(msg));
          }
        }
        catch (const std::exception& ex) {
          self->logger_->log(LogLevel::error, ex.what());
          self->fail(ec_errc(boost::system::errc::protocol_error));
          return;
        }

        self->do_read();
      }));
}

void Session::do_write()
{
  if (closed_)
    return;
  if (write_q_.empty()) {
    writing_ = false;
    return;
  }

  writing_ = true;
  arm_write_timeout();

  // On envoie le front ; il doit rester vivant jusqu'au handler => on garde une copie partagée.
  auto data = std::make_shared<std::vector<std::byte>>(std::move(write_q_.front()));
  write_q_.pop_front();

  // Ajuste compteur file
  write_q_bytes_ -= data->size();

  asio::async_write(
      socket_,
      asio::buffer(static_cast<const void*>(data->data()), data->size()),
      asio::bind_executor(
          strand_, [self = shared_from_this(), data](const ErrorCode& ec, std::size_t /*n*/) {
            if (ec) {
              self->fail(ec);
              return;
            }

            ErrorCode ignored;
            self->write_timer_.cancel(ignored);

            self->do_write();
          }));
}

void Session::arm_read_timeout()
{
  if (cfg_.read_timeout.count() <= 0)
    return;

  read_timer_.expires_after(cfg_.read_timeout);
  read_timer_.async_wait(
      asio::bind_executor(strand_, [self = shared_from_this()](const ErrorCode& ec) {
        if (ec)
          return; // cancelled
        self->logger_->log(LogLevel::warn, "read timeout");
        self->fail(ec_errc(boost::system::errc::timed_out));
      }));
}

void Session::arm_write_timeout()
{
  if (cfg_.write_timeout.count() <= 0)
    return;

  write_timer_.expires_after(cfg_.write_timeout);
  write_timer_.async_wait(
      asio::bind_executor(strand_, [self = shared_from_this()](const ErrorCode& ec) {
        if (ec)
          return; // cancelled
        self->logger_->log(LogLevel::warn, "write timeout");
        self->fail(ec_errc(boost::system::errc::timed_out));
      }));
}

void Session::cancel_timers()
{
  ErrorCode ignored;
  read_timer_.cancel(ignored);
  write_timer_.cancel(ignored);
}

void Session::fail(const ErrorCode& ec)
{
  if (closed_)
    return;
  closed_ = true;

  cancel_timers();

  ErrorCode ignored;
  socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ignored);
  socket_.close(ignored);

  if (on_disc_)
    on_disc_(shared_from_this(), ec);
}

} // namespace netlib
