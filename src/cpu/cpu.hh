#pragma once

#include "bus.hh"
#include "psr.hh"

#include <cstdint>

using std::size_t;

class Cpu {
  public:
    Cpu(Bus bus);

  private:
    static constexpr size_t GPR_FIQ_BANKED_FIRST = 8;
    static constexpr size_t GPR_FIQ_BANKED_COUNT = 7;

    static constexpr size_t GPR_SVC_BANKED_FIRST = 13;
    static constexpr size_t GPR_SVC_BANKED_COUNT = 2;

    static constexpr size_t GPR_ABT_BANKED_FIRST = 13;
    static constexpr size_t GPR_ABT_BANKED_COUNT = 2;

    static constexpr size_t GPR_IRQ_BANKED_FIRST = 13;
    static constexpr size_t GPR_IRQ_BANKED_COUNT = 2;

    static constexpr size_t GPR_UND_BANKED_FIRST = 13;
    static constexpr size_t GPR_UND_BANKED_COUNT = 2;

    static constexpr size_t GPR_SYS_USR_BANKED_FIRST = 8;
    static constexpr size_t GPR_SYS_USR_BANKED_COUNT = 7;

    static constexpr size_t GPR_VISIBLE_COUNT = 16;

    uint32_t gpr[GPR_VISIBLE_COUNT]; // general purpose registers
    Psr cpsr;                        // current program status register
    Psr spsr;                        // status program status register
    Bus bus;

    struct {
        uint32_t fiq[GPR_FIQ_BANKED_COUNT];
        uint32_t svc[GPR_SVC_BANKED_COUNT];
        uint32_t abt[GPR_ABT_BANKED_COUNT];
        uint32_t irq[GPR_IRQ_BANKED_COUNT];
        uint32_t und[GPR_UND_BANKED_COUNT];

        // visible registers before the mode switch
        uint32_t old[GPR_SYS_USR_BANKED_COUNT];
    } gpr_banked; // banked general purpose registers

    struct {
        Psr fiq;
        Psr svc;
        Psr abt;
        Psr irq;
        Psr und;
    } spsr_banked; // banked saved program status registers

    void chg_mode(Mode from, Mode to);

    uint32_t& operator[](uint8_t idx);
    const uint32_t& operator[](uint8_t idx) const;
};
