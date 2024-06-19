#include "cpu/cpu.hh"
#include "util/tcp_server.hh"

namespace matar {
class GdbRsp {
  public:
    GdbRsp(std::shared_ptr<Cpu> cpu, uint port);
    ~GdbRsp() = default;
    void start();
    void attach();
    void satisfy_client();
    void step();
    void step(std::string msg);
    void notify_breakpoint_reached();
    inline bool is_attached() { return attached; }

  private:
    bool attached = false;

    std::shared_ptr<Cpu> cpu;
    net::TcpServer server;
    std::string receive();
    std::string make_packet(std::string raw);

    bool ack_mode = true;
    void acknowledge();
    void send_empty();
    void send_ok();

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

    static constexpr uint MAX_MSG_LEN = 4096;
};
}
