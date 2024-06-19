#include "cpu/cpu.hh"
#include "cpu/arm/instruction.hh"
#include "cpu/thumb/instruction.hh"
#include "util/bits.hh"
#include "util/log.hh"

namespace matar {
Cpu::Cpu(std::shared_ptr<Bus> bus) noexcept
  : bus(bus) {
    cpsr.set_mode(Mode::Supervisor);
    cpsr.set_irq_disabled(true);
    cpsr.set_fiq_disabled(true);
    cpsr.set_state(State::Arm);
    glogger.info("CPU successfully initialised");

    // PC always points to two instructions ahead
    flush_pipeline();
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
              gpr.end() - 1,                                                   \
              gpr_banked.mode.begin())

    switch (from) {
        case Mode::Fiq:
            STORE_BANKED(fiq, FIQ);
            spsr_banked.fiq = spsr;
            std::copy(gpr_banked.old.begin(),
                      gpr_banked.old.end() - 2, // dont copy R13 and R14
                      gpr.begin() + GPR_OLD_FIRST);
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
            // we only take care of r13 and r14, because FIQ takes care of the
            // rest
            gpr_banked.old[5] = gpr[13];
            gpr_banked.old[6] = gpr[14];
            break;
    }

#undef STORE_BANKED

#define RESTORE_BANKED(mode, MODE)                                             \
    std::copy(gpr_banked.mode.begin(),                                         \
              gpr_banked.mode.end(),                                           \
              gpr.begin() + GPR_##MODE##_FIRST)

    switch (to) {
        case Mode::Fiq:
            RESTORE_BANKED(fiq, FIQ);
            spsr = spsr_banked.fiq;
            std::copy(gpr.begin() + GPR_FIQ_FIRST,
                      gpr.end() - 2, // dont copy R13 and R14
                      gpr_banked.old.begin());
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
            gpr[13] = gpr_banked.old[5];
            gpr[14] = gpr_banked.old[6];
            break;
    }

#undef RESTORE_BANKED

    cpsr.set_mode(to);
    glogger.info_bold("Mode changed from {:b} to {:b}",
                      static_cast<uint32_t>(from),
                      static_cast<uint32_t>(to));
}

void
Cpu::step() {
    // halfword align
    rst_bit(pc, 0);
    if (cpsr.state() == State::Arm) {
        // word align
        rst_bit(pc, 1);

        arm::Instruction instruction(opcodes[0]);

        opcodes[0] = opcodes[1];
        opcodes[1] = bus->read_word(pc, next_access);

#ifdef DISASSEMBLER
        glogger.info("0x{:08X} : {}",
                     pc - 2 * arm::INSTRUCTION_SIZE,
                     instruction.disassemble());
#endif

        instruction.exec(*this);

        if (is_flushed) {
            flush_pipeline();
            is_flushed = false;
        } else
            advance_pc_arm();
    } else {
        thumb::Instruction instruction(opcodes[0]);

        opcodes[0] = opcodes[1];
        opcodes[1] = bus->read_halfword(pc, next_access);

#ifdef DISASSEMBLER
        glogger.info("0x{:08X} : {}",
                     pc - 2 * thumb::INSTRUCTION_SIZE,
                     instruction.disassemble());
#endif

        instruction.exec(*this);

        if (is_flushed) {
            flush_pipeline();
            is_flushed = false;
        } else
            advance_pc_thumb();
    }
}

void
Cpu::flush_pipeline() {
    // halfword align
    rst_bit(pc, 0);
    if (cpsr.state() == State::Arm) {
        // word align
        rst_bit(pc, 1);
        opcodes[0] = bus->read_word(pc, CpuAccess::NonSequential);
        advance_pc_arm();
        opcodes[1] = bus->read_word(pc, CpuAccess::Sequential);
        advance_pc_arm();
    } else {
        opcodes[0] = bus->read_halfword(pc, CpuAccess::NonSequential);
        advance_pc_thumb();
        opcodes[1] = bus->read_halfword(pc, CpuAccess::Sequential);
        advance_pc_thumb();
    }
    next_access = CpuAccess::Sequential;
};
}
