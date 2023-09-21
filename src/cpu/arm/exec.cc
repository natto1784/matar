#include "cpu/cpu-impl.hh"
#include "util/bits.hh"
#include "util/log.hh"

using namespace logger;

namespace matar {
void
CpuImpl::exec_arm(const arm::Instruction instruction) {
    arm::Condition cond       = instruction.condition;
    arm::InstructionData data = instruction.data;

    debug(cpsr.condition(cond));
    if (!cpsr.condition(cond)) {
        return;
    }

    auto pc_error = [](uint8_t r) {
        if (r == PC_INDEX)
            log_error("Using PC (R15) as operand register");
    };

    auto pc_warn = [](uint8_t r) {
        if (r == PC_INDEX)
            log_warn("Using PC (R15) as operand register");
    };

    using namespace arm;

    std::visit(
      overloaded{
        [this, pc_warn](BranchAndExchange& data) {
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
        [this](Branch& data) {
            if (data.link)
                gpr[14] = pc - INSTRUCTION_SIZE;

            // data.offset accounts for two instructions ahead when
            // disassembling, so need to adjust
            pc = static_cast<int32_t>(pc) - 2 * INSTRUCTION_SIZE + data.offset;

            // pc is affected so flush the pipeline
            is_flushed = true;
        },
        [this, pc_error](Multiply& data) {
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
        [this, pc_error](MultiplyLong& data) {
            if (data.rdhi == data.rdlo || data.rdhi == data.rm ||
                data.rdlo == data.rm)
                log_error("rdhi, rdlo and rm are not distinct in {}",
                          typeid(data).name());

            pc_error(data.rdhi);
            pc_error(data.rdlo);
            pc_error(data.rm);
            pc_error(data.rs);

            if (data.uns) {
                auto cast = [](uint32_t x) -> uint64_t {
                    return static_cast<uint64_t>(x);
                };

                uint64_t eval = cast(gpr[data.rm]) * cast(gpr[data.rs]) +
                                (data.acc ? (cast(gpr[data.rdhi]) << 32) |
                                              cast(gpr[data.rdlo])
                                          : 0);

                gpr[data.rdlo] = bit_range(eval, 0, 31);
                gpr[data.rdhi] = bit_range(eval, 32, 63);

            } else {
                auto cast = [](uint32_t x) -> int64_t {
                    return static_cast<int64_t>(static_cast<int32_t>(x));
                };

                int64_t eval = cast(gpr[data.rm]) * cast(gpr[data.rs]) +
                               (data.acc ? (cast(gpr[data.rdhi]) << 32) |
                                             cast(gpr[data.rdlo])
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
        [](Undefined) { log_warn("Undefined instruction"); },
        [this, pc_error](SingleDataSwap& data) {
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
        [this, pc_warn, pc_error](SingleDataTransfer& data) {
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
                address -= 2 * INSTRUCTION_SIZE;

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
                if (data.rd == PC_INDEX)
                    address += INSTRUCTION_SIZE;

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
        [this, pc_warn, pc_error](HalfwordTransfer& data) {
            uint32_t address = gpr[data.rn];
            uint32_t offset  = 0;

            if (!data.pre && data.write)
                log_error("Write-back enabled with post-indexing in {}",
                          typeid(data).name());

            if (data.sign && !data.load)
                log_error("Signed data found in {}", typeid(data).name());

            if (data.write)
                pc_warn(data.rn);

            // offset is register number (4 bits) when not an immediate
            if (!data.imm) {
                pc_error(data.offset);
                offset = gpr[data.offset];
            } else {
                offset = data.offset;
            }

            // PC is always two instructions ahead
            if (data.rn == PC_INDEX)
                address -= 2 * INSTRUCTION_SIZE;

            if (data.pre)
                address += (data.up ? offset : -offset);

            // load
            if (data.load) {
                // signed
                if (data.sign) {
                    // halfword
                    if (data.half) {
                        gpr[data.rd] = bus->read_halfword(address);

                        // sign extend the halfword
                        gpr[data.rd] =
                          (static_cast<int32_t>(gpr[data.rd]) << 16) >> 16;

                        // byte
                    } else {
                        gpr[data.rd] = bus->read_byte(address);

                        // sign extend the byte
                        gpr[data.rd] =
                          (static_cast<int32_t>(gpr[data.rd]) << 24) >> 24;
                    }
                    // unsigned halfword
                } else if (data.half) {
                    gpr[data.rd] = bus->read_halfword(address);
                }
                // store
            } else {
                // take PC into consideration
                if (data.rd == PC_INDEX)
                    address += INSTRUCTION_SIZE;

                // halfword
                if (data.half)
                    bus->write_halfword(address, gpr[data.rd]);
            }

            if (!data.pre)
                address += (data.up ? offset : -offset);

            if (!data.pre || data.write)
                gpr[data.rn] = address;

            if (data.rd == PC_INDEX && data.load)
                is_flushed = true;
        },
        [this, pc_error](BlockDataTransfer& data) {
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
            else
                address -= alignment;

            if (!data.pre || data.write)
                gpr[data.rn] = address;

            if (data.load && get_bit(data.regs, PC_INDEX))
                is_flushed = true;

            // load back the original mode registers
            chg_mode(mode);
        },
        [this, pc_error](PsrTransfer& data) {
            if (data.spsr && cpsr.mode() == Mode::User) {
                log_error("Accessing SPSR in User mode in {}",
                          typeid(data).name());
            }

            Psr& psr = data.spsr ? spsr : cpsr;

            switch (data.type) {
                case PsrTransfer::Type::Mrs:
                    pc_error(data.operand);
                    gpr[data.operand] = psr.raw();
                    break;
                case PsrTransfer::Type::Msr:
                    pc_error(data.operand);

                    if (cpsr.mode() != Mode::User) {
                        psr.set_all(gpr[data.operand]);
                    }
                    break;
                case PsrTransfer::Type::Msr_flg:
                    uint32_t operand =
                      (data.imm ? data.operand : gpr[data.operand]);
                    psr.set_n(get_bit(operand, 31));
                    psr.set_z(get_bit(operand, 30));
                    psr.set_c(get_bit(operand, 29));
                    psr.set_v(get_bit(operand, 28));
                    break;
            }
        },
        [this, pc_error](DataProcessing& data) {
            uint32_t op_1 = gpr[data.rn];
            uint32_t op_2 = 0;

            uint32_t result = 0;

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
                    op_1 += INSTRUCTION_SIZE;
            }

            bool overflow = cpsr.v();
            bool carry    = cpsr.c();

            auto sub = [&carry, &overflow](uint32_t a, uint32_t b) -> uint32_t {
                bool s1 = get_bit(a, 31);
                bool s2 = get_bit(b, 31);

                uint32_t result = a - b;

                carry    = b <= a;
                overflow = s1 != s2 && s2 == get_bit(result, 31);
                return result;
            };

            auto add = [&carry, &overflow](
                         uint32_t a, uint32_t b, bool c = 0) -> uint32_t {
                bool s1 = get_bit(a, 31);
                bool s2 = get_bit(b, 31);

                // 33 bits
                uint64_t result_ = a + b + c;
                uint32_t result  = result_ & 0xFFFFFFFF;

                carry    = get_bit(result_, 32);
                overflow = s1 == s2 && s2 != get_bit(result, 31);
                return result;
            };

            auto sbc = [&carry,
                        &overflow](uint32_t a, uint32_t b, bool c) -> uint32_t {
                bool s1 = get_bit(a, 31);
                bool s2 = get_bit(b, 31);

                uint64_t result_ = a - b + c - 1;
                uint32_t result  = result_ & 0xFFFFFFFF;

                carry    = get_bit(result_, 32);
                overflow = s1 != s2 && s2 == get_bit(result, 31);
                return result;
            };

            switch (data.opcode) {
                case OpCode::AND:
                case OpCode::TST:
                    result = op_1 & op_2;
                    result = op_1 & op_2;
                    break;
                case OpCode::EOR:
                case OpCode::TEQ:
                    result = op_1 ^ op_2;
                    break;
                case OpCode::SUB:
                case OpCode::CMP:
                    result = sub(op_1, op_2);
                    break;
                case OpCode::RSB:
                    result = sub(op_2, op_1);
                    break;
                case OpCode::ADD:
                case OpCode::CMN:
                    result = add(op_1, op_2);
                    break;
                case OpCode::ADC:
                    result = add(op_1, op_2, carry);
                    break;
                case OpCode::SBC:
                    result = sbc(op_1, op_2, carry);
                    break;
                case OpCode::RSC:
                    result = sbc(op_2, op_1, carry);
                    break;
                case OpCode::ORR:
                    result = op_1 | op_2;
                    break;
                case OpCode::MOV:
                    result = op_2;
                    break;
                case OpCode::BIC:
                    result = op_1 & ~op_2;
                    break;
                case OpCode::MVN:
                    result = ~op_2;
                    break;
            }

            auto set_conditions = [this, carry, overflow, result]() {
                cpsr.set_c(carry);
                cpsr.set_v(overflow);
                cpsr.set_n(get_bit(result, 31));
                cpsr.set_z(result == 0);
            };

            if (data.set) {
                if (data.rd == PC_INDEX) {
                    if (cpsr.mode() == Mode::User)
                        log_error("Running {} in User mode",
                                  typeid(data).name());
                    spsr = cpsr;
                } else {
                    set_conditions();
                }
            }

            if (data.opcode == OpCode::TST || data.opcode == OpCode::TEQ ||
                data.opcode == OpCode::CMP || data.opcode == OpCode::CMN) {
                set_conditions();
            } else {
                gpr[data.rd] = result;
                if (data.rd == PC_INDEX || data.opcode == OpCode::MVN)
                    is_flushed = true;
            }
        },
        [this](SoftwareInterrupt) {
            chg_mode(Mode::Supervisor);
            pc   = 0x08;
            spsr = cpsr;
        },
        [](auto& data) {
            log_error("Unimplemented {} instruction", typeid(data).name());
        } },
      data);
}
}
