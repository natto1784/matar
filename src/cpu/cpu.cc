#include "cpu.hh"
#include "util/bits.hh"
#include "util/log.hh"
#include "utility.hh"
#include <algorithm>
#include <cstdio>

using namespace logger;

Cpu::Cpu(Bus& bus)
  : bus(std::make_shared<Bus>(bus))
  , gpr({ 0 })
  , cpsr(0)
  , spsr(0)
  , gpr_banked({ { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, { 0 } })
  , spsr_banked({ 0, 0, 0, 0, 0 }) {
    cpsr.set_mode(Mode::System);
    cpsr.set_irq_disabled(true);
    cpsr.set_fiq_disabled(true);
    cpsr.set_state(State::Arm);
    log_info("CPU successfully initialised");

    // PC is always two instructions ahead in the pipeline
    pc += 2 * ARM_INSTRUCTION_SIZE;
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
              gpr.begin() + GPR_COUNT - 1,                                     \
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
Cpu::exec_arm(const ArmInstruction instruction) {
    auto cond = instruction.get_condition();
    auto data = instruction.get_data();

    if (!cpsr.condition(cond)) {
        return;
    }

    auto pc_error = [](uint8_t r) {
        if (r == 15)
            log_error("Using PC (R15) as operand register");
    };

    auto pc_warn = [](uint8_t r) {
        if (r == 15)
            log_warn("Using PC (R15) as operand register");
    };

    std::visit(
      overloaded{
        [this, pc_warn](ArmInstruction::BranchAndExchange& data) {
            State state = static_cast<State>(data.rn & 1);

            pc_warn(data.rn);

            // set state
            cpsr.set_state(state);

            // copy to PC
            pc = gpr[data.rn];

            // ignore [1:0] bits for arm and 0 bit for thumb
            rst_nth_bit(pc, 0);

            if (state == State::Arm)
                rst_nth_bit(pc, 1);
        },
        [this](ArmInstruction::Branch& data) {
            auto offset = data.offset;
            // lsh 2 and sign extend the 26 bit offset to 32 bits
            offset <<= 2;

            if (get_nth_bit(offset, 25))
                offset |= 0xFC000000;

            if (data.link)
                gpr[14] = pc - ARM_INSTRUCTION_SIZE;

            pc += offset - ARM_INSTRUCTION_SIZE;
        },
        [this, pc_error](ArmInstruction::Multiply& data) {
            if (data.rd == data.rm)
                log_error("rd and rm are not distinct in {}",
                          typeid(data).name());

            pc_error(data.rd);
            pc_error(data.rd);
            pc_error(data.rd);

            gpr[data.rd] =
              gpr[data.rm] * gpr[data.rs] + (data.acc ? gpr[data.rn] : 0);

            if (data.set) {
                cpsr.set_z(!static_cast<bool>(gpr[data.rd]));
                cpsr.set_n(get_nth_bit(gpr[data.rd], 31));
                cpsr.set_c(0);
            }
        },
        [this, pc_error](ArmInstruction::MultiplyLong& data) {
            if (data.rdhi == data.rdlo || data.rdhi == data.rm ||
                data.rdlo == data.rm)
                log_error("rdhi, rdlo and rm are not distinct in {}",
                          typeid(data).name());

            pc_error(data.rdhi);
            pc_error(data.rdlo);
            pc_error(data.rm);
            pc_error(data.rs);
            if (data.uns) {
                uint64_t eval =
                  static_cast<uint64_t>(gpr[data.rm]) *
                    static_cast<uint64_t>(gpr[data.rs]) +
                  (data.acc ? static_cast<uint64_t>(gpr[data.rdhi]) << 32 |
                                static_cast<uint64_t>(gpr[data.rdlo])
                            : 0);

                gpr[data.rdlo] = get_bit_range(eval, 0, 31);
                gpr[data.rdhi] = get_bit_range(eval, 32, 63);

            } else {
                int64_t eval =
                  static_cast<int64_t>(gpr[data.rm]) *
                    static_cast<int64_t>(gpr[data.rs]) +
                  (data.acc ? static_cast<int64_t>(gpr[data.rdhi]) << 32 |
                                static_cast<int64_t>(gpr[data.rdlo])
                            : 0);

                gpr[data.rdlo] = get_bit_range(eval, 0, 31);
                gpr[data.rdhi] = get_bit_range(eval, 32, 63);
            }

            if (data.set) {
                cpsr.set_z(!(static_cast<bool>(gpr[data.rdhi]) ||
                             static_cast<bool>(gpr[data.rdlo])));
                cpsr.set_n(get_nth_bit(gpr[data.rdhi], 31));
                cpsr.set_c(0);
                cpsr.set_v(0);
            }
        },
        [](ArmInstruction::Undefined) { log_warn("Undefined instruction"); },
        [this, pc_warn](ArmInstruction::SingleDataSwap& data) {
            pc_warn(data.rm);
            pc_warn(data.rn);
            pc_warn(data.rd);

            if (data.byte) {
                gpr[data.rd] = bus->read_byte(gpr[data.rn]);
                bus->write_byte(gpr[data.rn], gpr[data.rm] & 0xFF);
            } else {
                gpr[data.rd] = bus->read_word(gpr[data.rn]);
                bus->write_word(gpr[data.rn], gpr[data.rm]);
            }
        },
        [this, pc_warn, pc_error](ArmInstruction::SingleDataTransfer& data) {
            uint32_t offset  = 0;
            uint32_t address = gpr[data.rn];

            if (!data.pre && data.write)
                log_warn("Write-back enabled with post-indexing in {}",
                         typeid(data).name());

            if (data.write)
                pc_warn(data.rn);

            // evaluate the offset
            if (const uint16_t* immediate =
                  std::get_if<uint16_t>(&data.offset)) {
                offset = *immediate;
            } else if (const Shift* shift = std::get_if<Shift>(&data.offset)) {
                uint8_t amount =
                  (shift->data.immediate ? shift->data.operand
                                         : gpr[shift->data.operand] & 0xFF);

                bool carry = cpsr.c();

                if (!shift->data.immediate)
                    pc_error(shift->data.operand);
                pc_error(shift->rm);

                eval_shift(shift->data.type, gpr[shift->rm], amount, carry);

                cpsr.set_c(carry);
            }

            // PC is always two instructions ahead
            if (data.rn == 15)
                address -= 2 * ARM_INSTRUCTION_SIZE;

            if (data.pre)
                address += (data.up ? offset : -offset);

            // load
            if (data.load) {
                // byte
                if (data.byte)
                    gpr[data.rd] = bus->read_byte(address);
                // word
                else
                    gpr[data.rd] = bus->read_word(address);
                // store
            } else {
                // take PC into consideration
                if (data.rd == 15)
                    address += ARM_INSTRUCTION_SIZE;

                // byte
                if (data.byte)
                    bus->write_byte(address, gpr[data.rd] & 0xFF);
                // word
                else
                    bus->write_word(address, gpr[data.rd]);
            }

            if (!data.pre)
                address += (data.up ? offset : -offset);

            if (!data.pre || data.write)
                gpr[data.rn] = address;
        },
        [this, pc_warn, pc_error](ArmInstruction::HalfwordTransfer& data) {
            uint32_t address = gpr[data.rn];

            if (!data.pre && data.write)
                log_error("Write-back enabled with post-indexing in {}",
                          typeid(data).name());

            if (data.sign && !data.load)
                log_error("Signed data found in {}", typeid(data).name());

            if (data.write)
                pc_warn(data.rn);

            // offset is register number (4 bits) when not an immediate
            if (!data.imm)
                pc_error(data.offset);

            if (data.pre)
                address += (data.up ? data.offset : -data.offset);

            // load
            if (data.load) {
                // signed
                if (data.sign) {
                    // halfword
                    if (data.half) {
                        gpr[data.rd] = bus->read_halfword(address);

                        // sign extend the halfword
                        if (get_nth_bit(gpr[data.rd], 15))
                            gpr[data.rd] |= 0xFFFF0000;
                        // byte
                    } else {
                        gpr[data.rd] = bus->read_byte(address);

                        // sign extend the byte
                        if (get_nth_bit(gpr[data.rd], 7))
                            gpr[data.rd] |= 0xFFFFFF00;
                    }
                    // unsigned halfword
                } else if (data.half) {
                    gpr[data.rd] = bus->read_halfword(address);
                }
                // store
            } else {
                // take PC into consideration
                if (data.rd == 15)
                    address += ARM_INSTRUCTION_SIZE;

                // halfword
                if (data.half)
                    bus->write_halfword(address, gpr[data.rd]);
            }

            if (!data.pre)
                address += (data.up ? data.offset : -data.offset);

            if (!data.pre || data.write)
                gpr[data.rn] = address;
        },
        [this](ArmInstruction::SoftwareInterrupt) {
            chg_mode(Mode::Supervisor);
            pc   = 0x08;
            spsr = cpsr;
        },
        [](auto& data) { log_error("{} instruction", typeid(data).name()); } },
      data);
}

void
Cpu::step() {
    uint32_t cur_pc = pc - 2 * ARM_INSTRUCTION_SIZE;

    if (cpsr.state() == State::Arm) {
        ArmInstruction instruction(bus->read_word(cur_pc));
        log_info("{:#034b}", bus->read_word(cur_pc));

        exec_arm(instruction);

        log_info("{:#010X} : {}", cur_pc, instruction.disassemble());

        pc += ARM_INSTRUCTION_SIZE;
    }
}
