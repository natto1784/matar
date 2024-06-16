#include "tcp_server.hh"

#include <cstring>
#include <format>
#include <sys/ioctl.h>
#include <unistd.h>

namespace net {
TcpServer::TcpServer()
  : server_fd(0)
  , client_fd(0) {}

TcpServer::~TcpServer() {
    close(server_fd);
    close(client_fd);
}

bool
TcpServer::client_waiting() {
    int count = 0;
    ioctl(client_fd, FIONREAD, &count);
    return static_cast<bool>(count);
}

void
TcpServer::run() {
    socklen_t cli_addr_size = sizeof(client_addr);

    client_fd = ::accept(
      server_fd, reinterpret_cast<sockaddr*>(&client_addr), &cli_addr_size);

    if (client_fd == -1)
        throw std::runtime_error("accept failed");
}

void
TcpServer::start(uint port) {
    server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        throw std::runtime_error("creating socket failed");
    }

    int option = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = PF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port        = htons(port);

    if (::bind(server_fd,
               reinterpret_cast<sockaddr*>(&server_addr),
               sizeof(server_addr)) == -1) {
        throw std::runtime_error("binding socket failed");
    }
    if (::listen(server_fd, 1) == -1) {
        throw std::runtime_error("listening failed");
    }
}

void
TcpServer::send(std::string msg) {
    if (::send(client_fd, msg.data(), msg.length(), 0) == -1) {
        throw std::runtime_error(
          std::format("failed to send message: {}\n", strerror(errno)));
    }
}

std::string
TcpServer::receive(uint length) {
    char msg[MAX_PACKET_SIZE];
    ssize_t num_bytes = recv(client_fd, msg, length, 0);
    msg[length]       = '\0';
    if (num_bytes < 0) {
        throw std::runtime_error(
          std::format("failed to receive messages: {}\n", strerror(errno)));
    }
    return std::string(msg);
}
}
