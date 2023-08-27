#pragma once

#include "bus.hh"
#include "psr.hh"

#include <cstdint>

using std::size_t;

static constexpr size_t GPR_VISIBLE_COUNT = 16;

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

struct _GprBanked {
    uint32_t fiq[GPR_FIQ_BANKED_COUNT];
    uint32_t svc[GPR_SVC_BANKED_COUNT];
    uint32_t abt[GPR_ABT_BANKED_COUNT];
    uint32_t irq[GPR_IRQ_BANKED_COUNT];
    uint32_t und[GPR_UND_BANKED_COUNT];

    /* visible registers before the mode switch */
    uint32_t old[GPR_SYS_USR_BANKED_COUNT];
};
typedef struct _GprBanked GprBanked;

struct _SpsrBanked {
    Psr fiq;
    Psr svc;
    Psr abt;
    Psr irq;
    Psr und;
};
typedef struct _SpsrBanked SpsrBanked;

class Cpu {
    uint32_t gpr[GPR_VISIBLE_COUNT]; // general purpose registers
    GprBanked gpr_banked;            // banked general purpose registers
    SpsrBanked spsr_banked;          // banked saved program status registers
    Psr cpsr;                        // current program status register
    Psr spsr;                        // status program status register
    Bus bus;

    void chg_mode(Mode from, Mode to);

    uint32_t& operator[](size_t idx);
    const uint32_t& operator[](size_t idx) const;

  public:
    Cpu(Bus bus)
      : gpr(0)
      , gpr_banked({ { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, { 0 } })
      , spsr_banked({ 0, 0, 0, 0, 0 })
      , cpsr(0)
      , spsr(0)
      , bus(bus) {
        cpsr.set_mode(Mode::System);
        cpsr.set_irq_disabled(true);
        cpsr.set_fiq_disabled(true);
        cpsr.set_state(State::Arm);
    }
};
