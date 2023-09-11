#include "cpu.hh"
#include "cpu/utility.hh"
#include <algorithm>
#include <cstdio>

Cpu::Cpu(Bus bus)
  : gpr(0)
  , cpsr(0)
  , spsr(0)
  , bus(bus)
  , gpr_banked({ { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, { 0 } })
  , spsr_banked({ 0, 0, 0, 0, 0 }) {
    cpsr.set_mode(Mode::System);
    cpsr.set_irq_disabled(true);
    cpsr.set_fiq_disabled(true);
    cpsr.set_state(State::Arm);
}

/* change modes */
void
Cpu::chg_mode(Mode from, Mode to) {
    if (from == to)
        return;

/* TODO: replace visible registers with view once I understand how to
 * concatenate views */
#define STORE_BANKED(mode, MODE)                                               \
    std::copy(gpr + GPR_##MODE##_BANKED_FIRST,                                 \
              gpr + GPR_##MODE##_BANKED_FIRST + GPR_##MODE##_BANKED_COUNT,     \
              gpr_banked.mode)

    switch (from) {
        case Mode::Fiq:
            STORE_BANKED(fiq, FIQ);
            spsr_banked.fiq = spsr;
            break;

        case Mode::Supervisor:
            STORE_BANKED(svc, SVC);
            spsr_banked.svc = spsr;
            break;

        case Mode::Abort:
            STORE_BANKED(abt, ABT);
            spsr_banked.abt = spsr;
            break;

        case Mode::Irq:
            STORE_BANKED(irq, IRQ);
            spsr_banked.irq = spsr;
            break;

        case Mode::Undefined:
            STORE_BANKED(und, UND);
            spsr_banked.und = spsr;
            break;

        case Mode::User:
        case Mode::System:
            STORE_BANKED(old, SYS_USR);
            break;
    }

#define RESTORE_BANKED(mode, MODE)                                             \
    std::copy(gpr_banked.mode,                                                 \
              gpr_banked.mode + GPR_##MODE##_BANKED_COUNT,                     \
              gpr + GPR_##MODE##_BANKED_FIRST)

    switch (to) {
        case Mode::Fiq:
            RESTORE_BANKED(fiq, FIQ);
            spsr = spsr_banked.fiq;
            break;

        case Mode::Supervisor:
            RESTORE_BANKED(svc, SVC);
            spsr = spsr_banked.svc;
            break;

        case Mode::Abort:
            RESTORE_BANKED(abt, ABT);
            spsr = spsr_banked.abt;
            break;

        case Mode::Irq:
            RESTORE_BANKED(irq, IRQ);
            spsr = spsr_banked.irq;
            break;

        case Mode::Undefined:
            RESTORE_BANKED(und, UND);
            spsr = spsr_banked.und;
            break;

        case Mode::User:
        case Mode::System:
            STORE_BANKED(old, SYS_USR);
            break;
    }

#undef RESTORE_BANKED

    cpsr.set_mode(to);
}

//  set register
inline uint32_t&
Cpu::operator[](uint8_t idx) {
    // avoid unneeded complexity like index checks
    return gpr[idx];
}

// get register
inline const uint32_t&
Cpu::operator[](uint8_t idx) const {
    return gpr[idx];
}
