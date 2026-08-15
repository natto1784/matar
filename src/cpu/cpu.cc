#include "cpu/cpu.hh"
#include "cpu/arm/instruction.hh"
#include "cpu/thumb/instruction.hh"
#include "util/bits.hh"
#include "util/log.hh"

namespace matar {
Cpu::Cpu(Bus& bus) noexcept
  : bus(bus) {
    cpsr.set_mode(Mode::Supervisor);
    cpsr.set_irq_disabled(true);
    cpsr.set_fiq_disabled(true);
    cpsr.set_state(State::Arm);
    glogger.info("CPU successfully initialised");

    bus.attach_cpu(this);

    // PC always points to two instructions ahead
    flush_pipeline();
}

/* change modes */
void
Cpu::chg_mode(const Mode to) {
    Mode from = cpsr.mode();

    if (from == to)
        return;

    switch (from) {
        case Mode::User:
        case Mode::System:
            gpr_banked.usr[5] = gpr[13];
            gpr_banked.usr[6] = gpr[14];
            spsr_banked.usr   = spsr;
            break;
        case Mode::Supervisor:
            gpr_banked.svc[0] = gpr[13];
            gpr_banked.svc[1] = gpr[14];
            spsr_banked.svc   = spsr;
            break;
        case Mode::Abort:
            gpr_banked.abt[0] = gpr[13];
            gpr_banked.abt[1] = gpr[14];
            spsr_banked.abt   = spsr;
            break;
        case Mode::Irq:
            gpr_banked.irq[0] = gpr[13];
            gpr_banked.irq[1] = gpr[14];
            spsr_banked.irq   = spsr;
            break;
        case Mode::Undefined:
            gpr_banked.und[0] = gpr[13];
            gpr_banked.und[1] = gpr[14];
            spsr_banked.und   = spsr;
            break;
        case Mode::Fiq:
            std::copy(
              gpr.begin() + 8, gpr.begin() + 15, gpr_banked.fiq.begin());
            std::copy(gpr_banked.usr.begin(),
                      gpr_banked.usr.begin() + 5,
                      gpr.begin() + 8);
            spsr_banked.fiq = spsr;
            break;
    }

    switch (to) {
        case Mode::User:
        case Mode::System:
            gpr[13] = gpr_banked.usr[5];
            gpr[14] = gpr_banked.usr[6];
            spsr    = spsr_banked.usr;
            break;
        case Mode::Supervisor:
            gpr[13] = gpr_banked.svc[0];
            gpr[14] = gpr_banked.svc[1];
            spsr    = spsr_banked.svc;
            break;
        case Mode::Abort:
            gpr[13] = gpr_banked.abt[0];
            gpr[14] = gpr_banked.abt[1];
            spsr    = spsr_banked.abt;
            break;
        case Mode::Irq:
            gpr[13] = gpr_banked.irq[0];
            gpr[14] = gpr_banked.irq[1];
            spsr    = spsr_banked.irq;
            break;
        case Mode::Undefined:
            gpr[13] = gpr_banked.und[0];
            gpr[14] = gpr_banked.und[1];
            spsr    = spsr_banked.und;
            break;
        case Mode::Fiq:
            std::copy(
              gpr.begin() + 8, gpr.begin() + 13, gpr_banked.usr.begin());
            std::copy(
              gpr_banked.fiq.begin(), gpr_banked.fiq.end(), gpr.begin() + 8);
            spsr = spsr_banked.fiq;
            break;
    }

    cpsr.set_mode(to);
    glogger.info("Mode changed from {:b} to {:b}",
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
        opcodes[1] = bus.read_word(pc, next_access);

#ifdef DISASSEMBLER
            glogger.info("0x{:08X} : {}",
                         pc - 2 * arm::INSTRUCTION_SIZE,
                         instruction.disassemble());
#endif

        exec(instruction);
    } else {
        thumb::Instruction instruction(opcodes[0]);

        opcodes[0] = opcodes[1];
        opcodes[1] = bus.read_halfword(pc, next_access);

#ifdef DISASSEMBLER
            glogger.info("0x{:08X} : {}",
                         pc - 2 * thumb::INSTRUCTION_SIZE,
                         instruction.disassemble());
#endif

        exec(instruction);
    }
}

void
Cpu::advance_pc_arm() {
    rst_bit(pc, 0);
    rst_bit(pc, 1);
    pc += arm::INSTRUCTION_SIZE;
};

void
Cpu::advance_pc_thumb() {
    rst_bit(pc, 0);
    pc += thumb::INSTRUCTION_SIZE;
}

void
Cpu::flush_pipeline() {
    rst_bit(pc, 0);
    if (cpsr.state() == State::Arm) {
        rst_bit(pc, 1);
        opcodes[0] = bus.read_word(pc, CpuAccess::NonSequential);
        advance_pc_arm();
        opcodes[1] = bus.read_word(pc, CpuAccess::Sequential);
        advance_pc_arm();
    } else {
        opcodes[0] = bus.read_halfword(pc, CpuAccess::NonSequential);
        advance_pc_thumb();
        opcodes[1] = bus.read_halfword(pc, CpuAccess::Sequential);
        advance_pc_thumb();
    }
    next_access = CpuAccess::Sequential;
}

void
Cpu::irq() {
    if (cpsr.irq_disabled()) {
        return;
    }

    spsr_banked.irq = cpsr;
    if (cpsr.state() == State::Thumb) {
        gpr_banked.irq[1] = pc - 2 * thumb::INSTRUCTION_SIZE + 4;
    } else {
        gpr_banked.irq[1] = pc - 2 * arm::INSTRUCTION_SIZE + 4;
    }
    chg_mode(Mode::Irq);
    cpsr.set_state(State::Arm);
    cpsr.set_irq_disabled(true);

    pc = IRQ_VECTOR;
    flush_pipeline();
}
}
