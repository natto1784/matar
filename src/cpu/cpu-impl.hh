#pragma once

#include "bus.hh"
#include "cpu/arm/instruction.hh"
#include "cpu/psr.hh"

#include <cstdint>

namespace matar {
class CpuImpl {
  public:
    CpuImpl(const Bus& bus) noexcept;

    void step();
    void chg_mode(const Mode to);
    void exec(const arm::Instruction instruction);

#ifndef MATAR_CPU_TESTS
  private:
#endif

    static constexpr uint8_t GPR_COUNT = 16;

    static constexpr uint8_t GPR_FIQ_FIRST     = 8;
    static constexpr uint8_t GPR_SVC_FIRST     = 13;
    static constexpr uint8_t GPR_ABT_FIRST     = 13;
    static constexpr uint8_t GPR_IRQ_FIRST     = 13;
    static constexpr uint8_t GPR_UND_FIRST     = 13;
    static constexpr uint8_t GPR_SYS_USR_FIRST = 8;

    std::shared_ptr<Bus> bus;
    std::array<uint32_t, GPR_COUNT> gpr; // general purpose registers

    Psr cpsr; // current program status register
    Psr spsr; // status program status register

    static constexpr uint8_t PC_INDEX = 15;
    static_assert(PC_INDEX < GPR_COUNT);

    uint32_t& pc = gpr[PC_INDEX];

    bool is_flushed;

    struct {
        std::array<uint32_t, GPR_COUNT - GPR_FIQ_FIRST - 1> fiq;
        std::array<uint32_t, GPR_COUNT - GPR_SVC_FIRST - 1> svc;
        std::array<uint32_t, GPR_COUNT - GPR_ABT_FIRST - 1> abt;
        std::array<uint32_t, GPR_COUNT - GPR_IRQ_FIRST - 1> irq;
        std::array<uint32_t, GPR_COUNT - GPR_UND_FIRST - 1> und;

        // visible registers before the mode switch
        std::array<uint32_t, GPR_COUNT - GPR_SYS_USR_FIRST> old;
    } gpr_banked; // banked general purpose registers

    struct {
        Psr fiq;
        Psr svc;
        Psr abt;
        Psr irq;
        Psr und;
    } spsr_banked; // banked saved program status registers
};
}
