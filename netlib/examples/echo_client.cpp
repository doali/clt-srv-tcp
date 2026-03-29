#include "netlib/net.hpp"
#include <iostream>
#include <string>

struct CoutLogger final : netlib::ILogger {
  void log(netlib::LogLevel, std::string_view msg) noexcept override {
    std::cout << "[netlib] " << msg << "\n";
  }
};

int main() {
  asio::io_context io;

  auto logger = std::make_shared<CoutLogger>();
  auto codec  = std::make_shared<netlib::IdentityCodec>();

  netlib::ClientConfig cfg;
  cfg.auto_reconnect = false;

  netlib::TcpClient cli(io, codec, logger, cfg);

  cli.connect(
    asio::ip::tcp::endpoint(asio::ip::make_address("127.0.0.1"), 5555),
    [](std::shared_ptr<netlib::Session>, netlib::ByteVec&& msg) {
      std::string s(reinterpret_cast<const char*>(msg.data()), msg.size());
      std::cout << "Received: " << s << "\n";
    },
    [](std::shared_ptr<netlib::Session>, const netlib::ErrorCode& ec) {
      std::cout << "Disconnected: " << ec.message() << "\n";
    }
  );

  asio::steady_timer t(io);
  t.expires_after(std::chrono::milliseconds(200));
  t.async_wait([&](const netlib::ErrorCode& ec) {
    if (ec) return;
    auto s = cli.session();
    if (!s) return;
    const std::string payload = "hello";
    s->async_send(std::span<const std::byte>(
      reinterpret_cast<const std::byte*>(payload.data()), payload.size()));
  });

  io.run();
}
