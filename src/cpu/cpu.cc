#include "cpu/cpu.hh"
#include "cpu/arm/instruction.hh"
#include "cpu/thumb/instruction.hh"
#include "util/bits.hh"
#include "util/log.hh"
#include <algorithm>
#include <cstdio>
#include <type_traits>

namespace matar {
Cpu::Cpu(const Bus& bus) noexcept
  : bus(std::make_shared<Bus>(bus))
  , gpr({ 0 })
  , cpsr(0)
  , spsr(0)
  , gpr_banked({ { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, { 0 } })
  , spsr_banked({ 0, 0, 0, 0, 0 })
  , is_flushed(false) {
    cpsr.set_mode(Mode::Supervisor);
    cpsr.set_irq_disabled(true);
    cpsr.set_fiq_disabled(true);
    cpsr.set_state(State::Arm);
    glogger.info("CPU successfully initialised");

    // PC always points to two instructions ahead
    // PC - 2 is the instruction being executed
    pc += 2 * arm::INSTRUCTION_SIZE;
}

/* change modes */
void
Cpu::chg_mode(const Mode to) {
    Mode from = cpsr.mode();

    if (from == to)
        return;

/* TODO: replace visible registers with view once I understand how to
 * concatenate views */
#define STORE_BANKED(mode, MODE)                                               \
    std::copy(gpr.begin() + GPR_##MODE##_FIRST,                                \
              gpr.begin() + gpr.size() - 1,                                    \
              gpr_banked.mode.begin())

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
    std::copy(gpr_banked.mode.begin(),                                         \
              gpr_banked.mode.end(),                                           \
              gpr.begin() + GPR_##MODE##_FIRST)

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

void
Cpu::step() {
    // Current instruction is two instructions behind PC
    uint32_t cur_pc = pc - 2 * arm::INSTRUCTION_SIZE;

    if (cpsr.state() == State::Arm) {
        arm::Instruction instruction(bus->read_word(cur_pc));

#ifdef DISASSEMBLER
        glogger.info("0x{:08X} : {}", cur_pc, instruction.disassemble());
#endif

        instruction.exec(*this);

    } else {
        thumb::Instruction instruction(bus->read_halfword(cur_pc));

#ifdef DISASSEMBLER
        glogger.info("0x{:08X} : {}", cur_pc, instruction.disassemble(cur_pc));
#endif

        instruction.exec(*this);
    }

    // advance PC
    {
        size_t size = cpsr.state() == State::Arm ? arm::INSTRUCTION_SIZE
                                                 : thumb::INSTRUCTION_SIZE;

        if (is_flushed) {
            // if flushed, do not increment the PC, instead set it to two
            // instructions ahead to account for flushed "fetch" and "decode"
            // instructions
            pc += 2 * size;
            is_flushed = false;
        } else {
            // if not flushed continue like normal
            pc += size;
        }
    }
}
}
