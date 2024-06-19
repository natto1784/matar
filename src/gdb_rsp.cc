#include "gdb_rsp.hh"
#include "util/log.hh"
#include <csignal>
#include <numeric>
#include <regex>
#include <stdexcept>
#include <string>

namespace matar {

template<typename... Args>
static inline constexpr void
gdb_log(const std::format_string<Args...>& fmt, Args&&... args) {
    glogger.debug("GDB: {}", std::format(fmt, std::forward<Args>(args)...));
}

static inline void
append_le(std::string& str, uint32_t value) {
    // little endian only
    str += std::format("{:02x}", value & 0xFF);
    str += std::format("{:02x}", value >> 8 & 0xFF);
    str += std::format("{:02x}", value >> 16 & 0xFF);
    str += std::format("{:02x}", value >> 24 & 0xFF);
}

static inline std::string
be_to_le(std::string str) {
    if (str.length() != 8)
        throw std::out_of_range("string is supposed to be 8 bytes");

    std::string current;

    for (int i = 7; i >= 0; i -= 2) {
        current += str[i - 1];
        current += str[i];
    }

    return current;
}

GdbRsp::GdbRsp(std::shared_ptr<Cpu> cpu, uint port)
  : cpu(cpu) {
    server.start(port);
}

void
GdbRsp::start() {
    server.run();

    attach();

    // attaching is not enough, we continue, until the last GDB communication
    // happens for ARMv4t i.e, fetching of the CPSR
    std::string msg;

    while (msg != "$p19") {
        msg = receive();
        step(msg); // 25th (0x19) register is cpsr
    }
}

void
GdbRsp::attach() {
    while (!attached) {
        step();
    }
}

void
GdbRsp::satisfy_client() {
    while (server.client_waiting() && attached) {
        step();
    }
}

void
GdbRsp::step() {
    std::string msg = receive();
    step(msg);
}

void
GdbRsp::step(std::string msg) {
    switch (msg[0]) {
        case '+':
            break;
        case '-':
            break;
        case '\x03':
            gdb_log("ctrl+c interrupt received");
            cmd_halted();
            break;
        case '$': {
            acknowledge();
            switch (msg[1]) {
                case '?':
                    cmd_halted();
                    break;
                case 'g':
                    cmd_read_registers();
                    break;
                case 'G':
                    cmd_write_registers(msg);
                    break;
                case 'p':
                    cmd_read_register(msg);
                    break;
                case 'P':
                    cmd_write_register(msg);
                    break;
                case 'm':
                    cmd_read_memory(msg);
                    break;
                case 'M':
                    cmd_write_memory(msg);
                    break;
                case 'z':
                    cmd_rm_breakpoint(msg);
                    break;
                case 'Z':
                    cmd_add_breakpoint(msg);
                    break;
                case 'c':
                    cmd_continue();
                    break;
                case 'D':
                    cmd_detach();
                    break;
                case 'Q':
                    if (msg == "$QStartNoAckMode")
                        ack_mode = true;
                    send_ok();
                    break;
                case 'q':
                    if (msg.starts_with("$qSupported")) {
                        cmd_supported(msg);
                        break;
                    } else if (msg == "$qAttached") {
                        cmd_attached();
                        break;
                    }
                    [[fallthrough]];
                default:
                    gdb_log("unknown command");
                    send_empty();
            }
            break;
        }
        default:
            gdb_log("unknown message received");
    }
}

std::string
GdbRsp::receive() {
    std::string msg = server.receive(1);
    char ch         = msg[0];
    uint checksum   = 0;

    if (ch == '$') {
        while ((ch = server.receive(1)[0]) != '#') {
            checksum += static_cast<uint>(ch);
            msg += ch;
            if (msg.length() > MAX_MSG_LEN) {
                throw std::logic_error("GDB: received message is too long");
            }
        }

        if (std::stoul(server.receive(2), nullptr, 16) != (checksum & 0xFF)) {
            gdb_log("{}", msg);
            throw std::logic_error("GDB: bad message checksum");
        }
    }

    gdb_log("received message \"{}\"", msg);
    return msg;
}

std::string
GdbRsp::make_packet(std::string raw) {
    uint checksum = std::accumulate(raw.begin(), raw.end(), 0);
    return std::format("${}#{:02x}", raw, checksum & 0xFF);
}

void
GdbRsp::acknowledge() {
    if (ack_mode)
        server.send("+");
}

void
GdbRsp::send_empty() {
    server.send(make_packet(""));
}

void
GdbRsp::send_ok() {
    acknowledge();
    server.send(make_packet("OK"));
}

void
GdbRsp::notify_breakpoint_reached() {
    gdb_log("reached breakpoint, sending signal");
    server.send(make_packet(std::format("S{:02x}", SIGTRAP)));
}

void
GdbRsp::cmd_attached() {
    attached = true;

    gdb_log("server is now attached");
    server.send(make_packet("1"));
}

void
GdbRsp::cmd_supported(std::string msg) {
    std::string response;

    if (msg.find("hwbreak+;") != std::string::npos)
        response += "hwbreak+;";

    // no acknowledgement mode
    response += "QStartNoAckMode+";

    gdb_log("sending response for qSupported");
    server.send(make_packet(response));
}

void
GdbRsp::cmd_halted() {
    gdb_log("sending reason for upcoming halt");
    server.send(make_packet(std::format("S{:02x}", SIGTRAP)));
}

void
GdbRsp::cmd_read_registers() {
    std::string response;

    for (int i = 0; i < cpu->GPR_COUNT - 1; i++)
        append_le(response, cpu->gpr[i]);

    // for some reason this PC needs to be the address of executing instruction
    // i.e, two instructions behind actual PC
    append_le(response,
              cpu->pc - 2 * (cpu->cpsr.state() == State::Arm
                               ? arm::INSTRUCTION_SIZE
                               : thumb::INSTRUCTION_SIZE));

    gdb_log("sending register values");
    server.send(make_packet(response));
}

void
GdbRsp::cmd_write_registers(std::string msg) {
    static std::regex rgx("\\$G([0-9A-Fa-f]+)");
    std::smatch sm;
    regex_match(msg, sm, rgx);

    if (sm.size() != 2 || sm[1].str().size() != 16 * 8) {
        gdb_log("invalid arguments to write registers");
        send_empty();
        return;
    }

    try {
        std::string values = sm[1].str();

        for (uint i = 0, j = 0; i < values.length() - 8; i += 8, j++) {
            cpu->gpr[i] = std::stoul(sm[i + 1].str(), nullptr, 16);
            cpu->gpr[j] =
              std::stoul(be_to_le(values.substr(i, 8)), nullptr, 16);
        }

        gdb_log("register values written");
        send_ok();
    } catch (const std::exception& e) {
        gdb_log("{}", e.what());
        send_empty();
    }
}

void
GdbRsp::cmd_read_register(std::string msg) {
    std::string response;

    try {
        uint reg = std::stoul(msg.substr(2), nullptr, 16);
        // 25th register is CPSR in gdb ARM
        if (reg == 25)
            append_le(response, cpu->cpsr.raw());
        else if (reg < cpu->GPR_COUNT)
            append_le(response, cpu->gpr[reg]);
        else
            response += "xxxxxxxx";

        gdb_log("sending single register value");
        server.send(make_packet(response));
    } catch (const std::exception& e) {
        gdb_log("{}", e.what());
        send_empty();
    }
}

void
GdbRsp::cmd_write_register(std::string msg) {
    static std::regex rgx("\\$P([0-9A-Fa-f]+)\\=([0-9A-Fa-f]+)");
    std::smatch sm;
    regex_match(msg, sm, rgx);

    if (sm.size() != 3 && sm[2].str().length() != 8) {
        gdb_log("invalid arguments to write single register");
        send_empty();
        return;
    }

    try {
        uint reg       = std::stoul(sm[1].str(), nullptr, 16);
        uint32_t value = std::stoul(be_to_le(sm[2].str()), nullptr, 16);

        dbg(value);

        if (reg == 25)
            cpu->cpsr.set_all(value);
        else if (reg < cpu->GPR_COUNT)
            cpu->gpr[reg] = value;

        gdb_log("single register value written");
        send_ok();
    } catch (const std::exception& e) {
        gdb_log("{}", e.what());
        send_empty();
    }
}

void
GdbRsp::cmd_read_memory(std::string msg) {
    std::string response;

    static std::regex rgx("\\$m([0-9A-Fa-f]+),([0-9A-Fa-f]+)");
    std::smatch sm;
    regex_match(msg, sm, rgx);

    if (sm.size() != 3) {
        gdb_log("invalid arguments to read memory");
        send_empty();
        return;
    }

    uint32_t address = 0, length = 0;

    try {
        address = std::stoul(sm[1].str(), nullptr, 16);
        length  = std::stoul(sm[2].str(), nullptr, 16);
    } catch (const std::exception& e) {
        gdb_log("{}", e.what());
        send_empty();
        return;
    }

    for (uint i = 0; i < length; i++) {
        response += std::format("{:02x}", cpu->bus->read_byte(address + i));
    }

    gdb_log("sending memory values values");
    server.send(make_packet(response));
}

void
GdbRsp::cmd_write_memory(std::string msg) {
    static std::regex rgx("\\$M([0-9A-Fa-f]+),([0-9A-Fa-f]+):([0-9A-Fa-f]+)");
    std::smatch sm;
    regex_match(msg, sm, rgx);

    if (sm.size() != 4) {
        gdb_log("invalid arguments to write memory");
        send_empty();
        return;
    }

    try {
        uint32_t address = std::stoul(sm[1].str(), nullptr, 16);
        uint32_t length  = std::stoul(sm[2].str(), nullptr, 16);

        std::string values = sm[3].str();

        for (uint i = 0, j = 0; i < length && j < values.size(); i++, j += 2) {
            cpu->bus->write_byte(
              address + i, std::stoul(values.substr(j, 2), nullptr, 16) & 0xFF);
        }

        gdb_log("register values written");
        send_ok();
    } catch (const std::exception& e) {
        gdb_log("{}", e.what());
        send_empty();
    }
}

void
GdbRsp::cmd_rm_breakpoint(std::string msg) {
    static std::regex rgx("\\$z(0|1),([0-9A-Fa-f]+),(2|3|4)");
    std::smatch sm;
    regex_match(msg, sm, rgx);

    if (sm.size() != 4) {
        gdb_log("invalid arguments to remove breakpoint");
        send_empty();
        return;
    }

    if (sm[1].str() != "0" && sm[0].str() != "1") {
        gdb_log("unrecognized breakpoint type encountered");
        send_empty();
        return;
    }

    if (sm[3].str() != "3" && sm[3].str() != "4") {
        gdb_log("only 32 bit breakpoints supported");
        send_empty();
        return;
    }

    try {
        uint32_t address = std::stoul(sm[2].str(), nullptr, 16);

        cpu->breakpoints.erase(address);
        gdb_log("breakpoint {:#08x} removed", address);
        send_ok();
    } catch (const std::exception& e) {
        gdb_log("{}", e.what());
        send_empty();
    }
}

void
GdbRsp::cmd_add_breakpoint(std::string msg) {
    static std::regex rgx("\\$Z(0|1),([0-9A-Fa-f]+),(2|3|4)");
    std::smatch sm;
    regex_match(msg, sm, rgx);
    dbg(sm.size());
    dbg(sm[0].str());

    if (sm.size() != 4) {
        gdb_log("invalid arguments to add breakpoint");
        send_empty();
        return;
    }

    if (sm[1].str() != "0" && sm[0].str() != "1") {
        gdb_log("unrecognized breakpoint type encountered");
        send_empty();
        return;
    }

    if (sm[3].str() != "3" && sm[3].str() != "4") {
        gdb_log("only 32 bit breakpoints supported");
        send_empty();
        return;
    }

    try {
        uint32_t address = std::stoul(sm[2].str(), nullptr, 16);

        cpu->breakpoints.insert(address);
        gdb_log("breakpoint {:#08x} added", address);
        send_ok();
    } catch (const std::exception& e) {
        gdb_log("{}", e.what());
        send_empty();
    }
}

void
GdbRsp::cmd_detach() {
    attached = false;
    gdb_log("detached");
    send_ok();
}

void
GdbRsp::cmd_continue() {
    // what to do?
    gdb_log("cpu continued");
    send_ok();
}
}
