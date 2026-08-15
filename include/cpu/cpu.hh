#pragma once

#include "arm/instruction.hh"
#include "bus.hh"
#include "cpu/psr.hh"
#include "thumb/instruction.hh"
#include <cstdint>

#ifdef GDB_DEBUG
#include <unordered_set>
#endif

namespace matar {

#ifdef GDB_DEBUG
class GdbRsp;
#endif

class Cpu {
  public:
    Cpu(Bus& bus) noexcept;

    void step();
    void chg_mode(const Mode to);

    void exec(arm::Instruction& instruction);
    void exec(thumb::Instruction& instruction);

    uint32_t program_counter() const { return gpr[15]; };
    uint32_t opcode0() const { return opcodes[0]; };
    uint32_t opcode1() const { return opcodes[1]; };
    State state() const { return cpsr.state(); };

    void irq();

#ifdef GDB_DEBUG
    bool breakpoint_reached() {
        if (breakpoints.contains(pc - 2 * (cpsr.state() == State::Arm
                                             ? arm::INSTRUCTION_SIZE
                                             : thumb::INSTRUCTION_SIZE))) {
            return true;
        }
        return false;
    }
#endif
  private:
    static constexpr auto SWI_VECTOR = 0x8;
    static constexpr auto IRQ_VECTOR = 0x18;

    friend void arm::Instruction::exec(Cpu& cpu);
    friend void thumb::Instruction::exec(Cpu& cpu);

    static constexpr uint8_t GPR_COUNT = 16;

    Bus& bus;
    std::array<uint32_t, GPR_COUNT> gpr = {}; // general purpose registers

    Psr cpsr = {}; // current program status register
    Psr spsr = {}; // status program status register

    static constexpr uint8_t SP_INDEX = 13;
    static_assert(SP_INDEX < GPR_COUNT);
    uint32_t& sp = gpr[SP_INDEX];

    static constexpr uint8_t LR_INDEX = 14;
    static_assert(LR_INDEX < GPR_COUNT);
    uint32_t& lr = gpr[LR_INDEX];

    static constexpr uint8_t PC_INDEX = 15;
    static_assert(PC_INDEX < GPR_COUNT);
    uint32_t& pc = gpr[PC_INDEX];

    struct {
        std::array<uint32_t, 7> usr;
        std::array<uint32_t, 7> fiq;

        std::array<uint32_t, 2> svc;
        std::array<uint32_t, 2> abt;
        std::array<uint32_t, 2> irq;
        std::array<uint32_t, 2> und;
    } gpr_banked = {}; // banked general purpose registers

    struct {
        Psr usr;
        Psr fiq;
        Psr svc;
        Psr abt;
        Psr irq;
        Psr und;
    } spsr_banked = {}; // banked saved program status registers

    void internal_cycle() { bus.internal_cycle(); }

    // whether read is going to be sequential or not
    CpuAccess next_access = CpuAccess::Sequential;

    // raw instructions in the pipeline
    std::array<uint32_t, 2> opcodes = {};

    void advance_pc_arm();
    void advance_pc_thumb();
    void flush_pipeline();

    uint8_t read_byte(uint32_t address, CpuAccess access) {
        return bus.read_byte(address, access);
    }

    uint16_t read_halfword(uint32_t address, CpuAccess access) {
        return bus.read_halfword(address & ~0b1, access);
    }

    uint32_t read_rotated_halfword(uint32_t address, CpuAccess access) {
        uint32_t halfword = bus.read_halfword(address & ~0b1, access);
        uint8_t rotation  = (address & 0b1) * 8;
        return std::rotr(halfword, rotation);
    }

    uint32_t read_word(uint32_t address, CpuAccess access) {
        return bus.read_word(address & ~0b11, access);
    }

    uint32_t read_rotated_word(uint32_t address, CpuAccess access) {
        uint32_t word    = bus.read_word(address & ~0b11, access);
        uint8_t rotation = (address & 0b11) * 8;
        return std::rotr(word, rotation);
    }

    void write_byte(uint32_t address, uint8_t byte, CpuAccess access) {
        bus.write_byte(address, byte, access);
    }

    void write_halfword(uint32_t address, uint16_t halfword, CpuAccess access) {
        bus.write_halfword(address & ~0b1, halfword, access);
    }

    void write_word(uint32_t address, uint32_t word, CpuAccess access) {
        bus.write_word(address & ~0b11, word, access);
    }
#ifdef GDB_DEBUG
    friend class GdbRsp;
    std::unordered_set<uint32_t> breakpoints = {};
#endif
};
}
