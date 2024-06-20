#pragma once

#include "arm/instruction.hh"
#include "bus.hh"
#include "cpu/psr.hh"
#include "thumb/instruction.hh"
#include <cstdint>
#include <memory>

#ifdef GDB_DEBUG
#include <unordered_set>
#endif

namespace matar {

#ifdef GDB_DEBUG
class GdbRsp;
#endif

class Cpu {
  public:
    Cpu(std::shared_ptr<Bus> bus) noexcept;

    void step();
    void chg_mode(const Mode to);

    void exec(arm::Instruction& instruction);
    void exec(thumb::Instruction& instruction);

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
    friend void arm::Instruction::exec(Cpu& cpu);
    friend void thumb::Instruction::exec(Cpu& cpu);

    static constexpr uint8_t GPR_COUNT = 16;

    static constexpr uint8_t GPR_FIQ_FIRST = 8;
    static constexpr uint8_t GPR_SVC_FIRST = 13;
    static constexpr uint8_t GPR_ABT_FIRST = 13;
    static constexpr uint8_t GPR_IRQ_FIRST = 13;
    static constexpr uint8_t GPR_UND_FIRST = 13;
    static constexpr uint8_t GPR_OLD_FIRST = 8;

    std::shared_ptr<Bus> bus;
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
        std::array<uint32_t, GPR_COUNT - GPR_FIQ_FIRST - 1> fiq;
        std::array<uint32_t, GPR_COUNT - GPR_SVC_FIRST - 1> svc;
        std::array<uint32_t, GPR_COUNT - GPR_ABT_FIRST - 1> abt;
        std::array<uint32_t, GPR_COUNT - GPR_IRQ_FIRST - 1> irq;
        std::array<uint32_t, GPR_COUNT - GPR_UND_FIRST - 1> und;

        // visible registers before the mode switch
        std::array<uint32_t, GPR_COUNT - GPR_OLD_FIRST - 1> old;
    } gpr_banked = {}; // banked general purpose registers

    struct {
        Psr fiq;
        Psr svc;
        Psr abt;
        Psr irq;
        Psr und;
    } spsr_banked = {}; // banked saved program status registers

    void internal_cycle() { bus->internal_cycle(); }

    // whether read is going to be sequential or not
    CpuAccess next_access = CpuAccess::Sequential;

    // raw instructions in the pipeline
    std::array<uint32_t, 2> opcodes = {};

    void advance_pc_arm();
    void advance_pc_thumb();

    void flush_pipeline();

#ifdef GDB_DEBUG
    friend class GdbRsp;
    std::unordered_set<uint32_t> breakpoints = {};
#endif
};
}
