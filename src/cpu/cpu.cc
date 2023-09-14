#include "cpu/cpu.hh"
#include "cpu/utility.hh"
#include "util/bits.hh"
#include "util/log.hh"
#include <algorithm>
#include <cstdio>

using namespace logger;

Cpu::Cpu(Bus& bus)
  : bus(std::make_shared<Bus>(bus))
  , gpr({ 0 })
  , cpsr(0)
  , spsr(0)
  , is_flushed(false)
  , gpr_banked({ { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, { 0 } })
  , spsr_banked({ 0, 0, 0, 0, 0 }) {
    cpsr.set_mode(Mode::Supervisor);
    cpsr.set_irq_disabled(true);
    cpsr.set_fiq_disabled(true);
    cpsr.set_state(State::Arm);
    log_info("CPU successfully initialised");

    // PC always points to two instructions ahead
    // PC - 2 is the instruction being executed
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
            rst_bit(pc, 0);

            if (state == State::Arm)
                rst_bit(pc, 1);

            // pc is affected so flush the pipeline
            is_flushed = true;
        },
        [this](ArmInstruction::Branch& data) {
            if (data.link)
                gpr[14] = pc - ARM_INSTRUCTION_SIZE;

            // data.offset accounts for two instructions ahead when
            // disassembling, so need to adjust
            pc =
              static_cast<int32_t>(pc) - 2 * ARM_INSTRUCTION_SIZE + data.offset;

            // pc is affected so flush the pipeline
            is_flushed = true;
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
                cpsr.set_z(gpr[data.rd] == 0);
                cpsr.set_n(get_bit(gpr[data.rd], 31));
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

                gpr[data.rdlo] = bit_range(eval, 0, 31);
                gpr[data.rdhi] = bit_range(eval, 32, 63);

            } else {
                int64_t eval =
                  static_cast<int64_t>(gpr[data.rm]) *
                    static_cast<int64_t>(gpr[data.rs]) +
                  (data.acc ? static_cast<int64_t>(gpr[data.rdhi]) << 32 |
                                static_cast<int64_t>(gpr[data.rdlo])
                            : 0);

                gpr[data.rdlo] = bit_range(eval, 0, 31);
                gpr[data.rdhi] = bit_range(eval, 32, 63);
            }

            if (data.set) {
                cpsr.set_z(gpr[data.rdhi] == 0 && gpr[data.rdlo] == 0);
                cpsr.set_n(get_bit(gpr[data.rdhi], 31));
                cpsr.set_c(0);
                cpsr.set_v(0);
            }
        },
        [](ArmInstruction::Undefined) { log_warn("Undefined instruction"); },
        [this, pc_error](ArmInstruction::SingleDataSwap& data) {
            pc_error(data.rm);
            pc_error(data.rn);
            pc_error(data.rd);

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

            if (data.rn == PC_INDEX && data.write)
                log_warn("Write-back enabled with base register as PC {}",
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

                offset =
                  eval_shift(shift->data.type, gpr[shift->rm], amount, carry);

                cpsr.set_c(carry);
            }

            // PC is always two instructions ahead
            if (data.rn == PC_INDEX)
                address -= 2 * ARM_INSTRUCTION_SIZE;

            if (data.pre)
                address += (data.up ? offset : -offset);

            debug(address);

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
                if (data.rd == PC_INDEX)
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

            if (data.rd == PC_INDEX && data.load)
                is_flushed = true;
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
                        if (get_bit(gpr[data.rd], PC_INDEX))
                            gpr[data.rd] |= 0xFFFF0000;
                        // byte
                    } else {
                        gpr[data.rd] = bus->read_byte(address);

                        // sign extend the byte
                        if (get_bit(gpr[data.rd], 7))
                            gpr[data.rd] |= 0xFFFFFF00;
                    }
                    // unsigned halfword
                } else if (data.half) {
                    gpr[data.rd] = bus->read_halfword(address);
                }
                // store
            } else {
                // take PC into consideration
                if (data.rd == PC_INDEX)
                    address += ARM_INSTRUCTION_SIZE;

                // halfword
                if (data.half)
                    bus->write_halfword(address, gpr[data.rd]);
            }

            if (!data.pre)
                address += (data.up ? data.offset : -data.offset);

            if (!data.pre || data.write)
                gpr[data.rn] = address;

            if (data.rd == PC_INDEX && data.load)
                is_flushed = true;
        },
        [this, pc_error](ArmInstruction::BlockDataTransfer& data) {
            uint32_t address  = gpr[data.rn];
            Mode mode         = cpsr.mode();
            uint8_t alignment = 4; // word
            uint8_t i         = 0;
            uint8_t n_regs    = std::popcount(data.regs);

            pc_error(data.rn);

            if (cpsr.mode() == Mode::User && data.s) {
                log_error("Bit S is set outside priviliged modes in {}",
                          typeid(data).name());
            }

            // we just change modes to load user registers
            if ((!get_bit(data.regs, PC_INDEX) && data.s) ||
                (!data.load && data.s)) {
                chg_mode(Mode::User);

                if (data.write) {
                    log_error("Write-back enable for user bank registers in {}",
                              typeid(data).name());
                }
            }

            // account for decrement
            if (!data.up)
                address -= (n_regs - 1) * alignment;

            if (data.pre)
                address += (data.up ? alignment : -alignment);

            if (data.load) {
                if (get_bit(data.regs, PC_INDEX) && data.s && data.load) {
                    // current mode's spsr is already loaded when it was
                    // switched
                    spsr = cpsr;
                }

                for (i = 0; i < GPR_COUNT; i++) {
                    if (get_bit(data.regs, i)) {
                        gpr[i] = bus->read_word(address);
                        address += alignment;
                    }
                }
            } else {
                for (i = 0; i < GPR_COUNT; i++) {
                    if (get_bit(data.regs, i)) {
                        bus->write_word(address, gpr[i]);
                        address += alignment;
                    }
                }
            }

            if (!data.pre)
                address += (data.up ? alignment : -alignment);

            // reset back to original address + offset if incremented earlier
            if (data.up)
                address -= n_regs * alignment;

            if (!data.pre || data.write)
                gpr[data.rn] = address;

            if (data.load && get_bit(data.regs, PC_INDEX))
                is_flushed = true;

            // load back the original mode registers
            chg_mode(mode);
        },
        [this, pc_error](ArmInstruction::PsrTransfer& data) {
            if (data.spsr && cpsr.mode() == Mode::User) {
                log_error("Accessing SPSR in User mode in {}",
                          typeid(data).name());
            }

            Psr& psr = data.spsr ? spsr : cpsr;

            switch (data.type) {
                case ArmInstruction::PsrTransfer::Type::Mrs:
                    pc_error(data.operand);
                    gpr[data.operand] = psr.raw();
                    break;
                case ArmInstruction::PsrTransfer::Type::Msr:
                    pc_error(data.operand);

                    if (cpsr.mode() != Mode::User) {
                        psr.set_all(gpr[data.operand]);
                        break;
                    }
                case ArmInstruction::PsrTransfer::Type::Msr_flg:
                    psr.set_n(get_bit(data.operand, 31));
                    psr.set_z(get_bit(data.operand, 30));
                    psr.set_c(get_bit(data.operand, 29));
                    psr.set_v(get_bit(data.operand, 28));
                    break;
            }
        },
        [this, pc_error](ArmInstruction::DataProcessing& data) {
            uint32_t op_1 = gpr[data.rn];
            uint32_t op_2 = 0;

            uint32_t result = 0;

            bool overflow = cpsr.v();
            bool carry    = cpsr.c();
            bool negative = cpsr.n();
            bool zero     = cpsr.z();

            if (const uint32_t* immediate =
                  std::get_if<uint32_t>(&data.operand)) {
                op_2 = *immediate;
            } else if (const Shift* shift = std::get_if<Shift>(&data.operand)) {
                uint8_t amount =
                  (shift->data.immediate ? shift->data.operand
                                         : gpr[shift->data.operand] & 0xFF);

                bool carry = cpsr.c();

                if (!shift->data.immediate)
                    pc_error(shift->data.operand);
                pc_error(shift->rm);

                op_2 =
                  eval_shift(shift->data.type, gpr[shift->rm], amount, carry);

                cpsr.set_c(carry);

                // PC is 12 bytes ahead when shifting
                if (data.rn == PC_INDEX)
                    op_1 += ARM_INSTRUCTION_SIZE;
            }

            switch (data.opcode) {
                case OpCode::AND: {
                    result = op_1 & op_2;

                    negative = get_bit(result, 31);
                } break;
                case OpCode::EOR: {
                    result = op_1 ^ op_2;

                    negative = get_bit(result, 31);
                } break;
                case OpCode::SUB: {
                    bool s1  = get_bit(op_1, 31);
                    bool s2  = get_bit(op_2, 31);
                    result   = op_1 - op_2;
                    negative = get_bit(result, 31);
                    carry    = op_1 < op_2;
                    overflow = s1 != s2 && s2 == negative;
                } break;
                case OpCode::RSB: {
                    bool s1 = get_bit(op_1, 31);
                    bool s2 = get_bit(op_2, 31);
                    result  = op_2 - op_1;

                    negative = get_bit(result, 31);
                    carry    = op_2 < op_1;
                    overflow = s1 != s2 && s1 == negative;
                } break;
                case OpCode::ADD: {
                    bool s1 = get_bit(op_1, 31);
                    bool s2 = get_bit(op_2, 31);

                    // result_ is 33 bits
                    uint64_t result_ = op_2 + op_1;
                    result           = result_ & 0xFFFFFFFF;

                    negative = get_bit(result, 31);
                    carry    = get_bit(result_, 32);
                    overflow = s1 == s2 && s1 != negative;
                } break;
                case OpCode::ADC: {
                    bool s1 = get_bit(op_1, 31);
                    bool s2 = get_bit(op_2, 31);

                    uint64_t result_ = op_2 + op_1 + carry;
                    result           = result_ & 0xFFFFFFFF;

                    negative = get_bit(result, 31);
                    carry    = get_bit(result_, 32);
                    overflow = s1 == s2 && s1 != negative;
                } break;
                case OpCode::SBC: {
                    bool s1 = get_bit(op_1, 31);
                    bool s2 = get_bit(op_2, 31);

                    uint64_t result_ = op_1 - op_2 + carry - 1;
                    result           = result_ & 0xFFFFFFFF;

                    negative = get_bit(result, 31);
                    carry    = get_bit(result_, 32);
                    overflow = s1 != s2 && s2 == negative;
                } break;
                case OpCode::RSC: {
                    bool s1 = get_bit(op_1, 31);
                    bool s2 = get_bit(op_2, 31);

                    uint64_t result_ = op_1 - op_2 + carry - 1;
                    result           = result_ & 0xFFFFFFFF;

                    negative = get_bit(result, 31);
                    carry    = get_bit(result_, 32);
                    overflow = s1 != s2 && s1 == negative;
                } break;
                case OpCode::TST: {
                    result = op_1 & op_2;

                    negative = get_bit(result, 31);
                } break;
                case OpCode::TEQ: {
                    result = op_1 ^ op_2;

                    negative = get_bit(result, 31);
                } break;
                case OpCode::CMP: {
                    bool s1 = get_bit(op_1, 31);
                    bool s2 = get_bit(op_2, 31);

                    result = op_1 - op_2;

                    negative = get_bit(result, 31);
                    carry    = op_1 < op_2;
                    overflow = s1 != s2 && s2 == negative;
                } break;
                case OpCode::CMN: {
                    bool s1 = get_bit(op_1, 31);
                    bool s2 = get_bit(op_2, 31);

                    uint64_t result_ = op_2 + op_1;
                    result           = result_ & 0xFFFFFFFF;

                    negative = get_bit(result, 31);
                    carry    = get_bit(result_, 32);
                    overflow = s1 == s2 && s1 != negative;
                } break;
                case OpCode::ORR: {
                    result = op_1 | op_2;

                    negative = get_bit(result, 31);
                } break;
                case OpCode::MOV: {
                    result = op_2;

                    negative = get_bit(result, 31);
                } break;
                case OpCode::BIC: {
                    result = op_1 & ~op_2;

                    negative = get_bit(result, 31);
                } break;
                case OpCode::MVN: {
                    result = ~op_2;

                    negative = get_bit(result, 31);
                } break;
            }

            zero = result == 0;

            debug(carry);
            debug(overflow);
            debug(zero);
            debug(negative);

            auto set_conditions = [=]() {
                cpsr.set_c(carry);
                cpsr.set_v(overflow);
                cpsr.set_n(negative);
                cpsr.set_z(zero);
            };

            if (data.set) {
                if (data.rd == 15) {
                    if (cpsr.mode() == Mode::User)
                        log_error("Running {} in User mode",
                                  typeid(data).name());
                } else {
                    set_conditions();
                }
            }

            if (data.opcode == OpCode::TST || data.opcode == OpCode::TEQ ||
                data.opcode == OpCode::CMP || data.opcode == OpCode::CMN) {
                set_conditions();
            } else {
                gpr[data.rd] = result;
                if (data.rd == 15 || data.opcode == OpCode::MVN)
                    is_flushed = true;
            }
        },
        [this](ArmInstruction::SoftwareInterrupt) {
            chg_mode(Mode::Supervisor);
            pc   = 0x08;
            spsr = cpsr;
        },
        [](auto& data) {
            log_error("Unimplemented {} instruction", typeid(data).name());
        } },
      data);
}

void
Cpu::step() {
    // Current instruction is two instructions behind PC
    uint32_t cur_pc = pc - 2 * ARM_INSTRUCTION_SIZE;

    if (cpsr.state() == State::Arm) {
        debug(cur_pc);
        uint32_t x = bus->read_word(cur_pc);
        ArmInstruction instruction(x);
        log_info("{:#034b}", x);

        exec_arm(instruction);

        log_info("0x{:08X} : {}", cur_pc, instruction.disassemble());

        if (is_flushed) {
            // if flushed, do not increment the PC, instead set it to two
            // instructions ahead to account for flushed "fetch" and "decode"
            // instructions
            pc += 2 * ARM_INSTRUCTION_SIZE;
            is_flushed = false;
        } else {
            // if not flushed continue like normal
            pc += ARM_INSTRUCTION_SIZE;
        }
    }
}
