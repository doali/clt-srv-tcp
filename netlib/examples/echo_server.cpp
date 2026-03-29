#include "netlib/net.hpp"
#include <iostream>

struct CoutLogger final : netlib::ILogger {
  void log(netlib::LogLevel, std::string_view msg) noexcept override {
    std::cout << "[netlib] " << msg << "\n";
  }
};

int main() {
  asio::io_context io;

  auto logger = std::make_shared<CoutLogger>();
  auto codec  = std::make_shared<netlib::IdentityCodec>();

  netlib::TcpServer srv(io,
                        asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 5555),
                        codec, logger, {});

  srv.start(
    [](std::shared_ptr<netlib::Session> s) {
      netlib::ErrorCode ec;
      auto ep = s->remote_endpoint(ec);
      if (!ec) std::cout << "Client connected: " << ep.address().to_string()
                         << ":" << ep.port() << "\n";
    },
    [](std::shared_ptr<netlib::Session> s, netlib::ByteVec&& msg) {
      s->async_send(msg); // echo
    },
    [](std::shared_ptr<netlib::Session>, const netlib::ErrorCode& ec) {
      std::cout << "Client disconnected: " << ec.message() << "\n";
    }
  );

  std::cout << "Echo server listening on 0.0.0.0:5555\n";
  io.run();
}
