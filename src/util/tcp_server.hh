#pragma once

#include <netinet/in.h>
#include <string>

namespace net {

class TcpServer {
  public:
    TcpServer();
    ~TcpServer();

    void run();
    void start(uint port);
    void send(std::string msg);
    std::string receive(uint length);
    std::string receive_all() { return receive(MAX_PACKET_SIZE); };
    bool client_waiting();

  private:
    static constexpr uint MAX_PACKET_SIZE = 4096;
    char msg[MAX_PACKET_SIZE];
    int server_fd;
    int client_fd;
    sockaddr_in server_addr;
    sockaddr_in client_addr;
};
}
