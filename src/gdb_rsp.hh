#include "cpu/cpu.hh"
#include "util/tcp_server.hh"

namespace matar {
class GdbRsp {
  public:
    GdbRsp(std::shared_ptr<Cpu> cpu);
    void start(const uint port);
    void satisfy_client();
    void step();

  private:
    bool attached = false;

    std::shared_ptr<Cpu> cpu;
    net::TcpServer server;
    std::string receive();
    std::string make_packet(std::string raw);
    void acknowledge();
    void send_empty();
    void notify_breakpoint_reached();

    // Commands
    void cmd_attached();
    void cmd_supported(std::string msg);
    void cmd_halted();
    void cmd_read_registers();
    void cmd_write_registers(std::string msg);
    void cmd_read_register(std::string msg);
    void cmd_write_register(std::string msg);
    void cmd_read_memory(std::string msg);
    void cmd_write_memory(std::string msg);
    void cmd_rm_breakpoint(std::string msg);
    void cmd_add_breakpoint(std::string msg);
    void cmd_detach();
    void cmd_continue();

    static constexpr std::string ATTACHED_MSG = "$qAttached#8f";
    static constexpr std::string OK_MSG       = "+$OK#9a";

    static constexpr uint MAX_MSG_LEN = 4096;
};
}
