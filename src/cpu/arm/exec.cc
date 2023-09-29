#include "cpu/cpu-impl.hh"
#include "util/bits.hh"
#include "util/log.hh"

namespace matar::arm {
void
Instruction::exec(CpuImpl& cpu) {
    if (!cpu.cpsr.condition(condition)) {
        return;
    }

    auto pc_error = [cpu](uint8_t r) {
        if (r == cpu.PC_INDEX)
            glogger.error("Using PC (R15) as operand register");
    };

    auto pc_warn = [cpu](uint8_t r) {
        if (r == cpu.PC_INDEX)
            glogger.warn("Using PC (R15) as operand register");
    };

    using namespace arm;

    std::visit(
      overloaded{
        [&cpu, pc_warn](BranchAndExchange& data) {
            State state = static_cast<State>(data.rn & 1);

            pc_warn(data.rn);

            // set state
            cpu.cpsr.set_state(state);

            // copy to PC
            cpu.pc = cpu.gpr[data.rn];

            // ignore [1:0] bits for arm and 0 bit for thumb
            rst_bit(cpu.pc, 0);

            if (state == State::Arm)
                rst_bit(cpu.pc, 1);

            // pc is affected so flush the pipeline
            cpu.is_flushed = true;
        },
        [&cpu](Branch& data) {
            if (data.link)
                cpu.gpr[14] = cpu.pc - INSTRUCTION_SIZE;

            // data.offset accounts for two instructions ahead when
            // disassembling, so need to adjust
            cpu.pc =
              static_cast<int32_t>(cpu.pc) - 2 * INSTRUCTION_SIZE + data.offset;

            // pc is affected so flush the pipeline
            cpu.is_flushed = true;
        },
        [&cpu, pc_error](Multiply& data) {
            if (data.rd == data.rm)
                glogger.error("rd and rm are not distinct in {}",
                              typeid(data).name());

            pc_error(data.rd);
            pc_error(data.rd);
            pc_error(data.rd);

            cpu.gpr[data.rd] = cpu.gpr[data.rm] * cpu.gpr[data.rs] +
                               (data.acc ? cpu.gpr[data.rn] : 0);

            if (data.set) {
                cpu.cpsr.set_z(cpu.gpr[data.rd] == 0);
                cpu.cpsr.set_n(get_bit(cpu.gpr[data.rd], 31));
                cpu.cpsr.set_c(0);
            }
        },
        [&cpu, pc_error](MultiplyLong& data) {
            if (data.rdhi == data.rdlo || data.rdhi == data.rm ||
                data.rdlo == data.rm)
                glogger.error("rdhi, rdlo and rm are not distinct in {}",
                              typeid(data).name());

            pc_error(data.rdhi);
            pc_error(data.rdlo);
            pc_error(data.rm);
            pc_error(data.rs);

            if (data.uns) {
                auto cast = [](uint32_t x) -> uint64_t {
                    return static_cast<uint64_t>(x);
                };

                uint64_t eval =
                  cast(cpu.gpr[data.rm]) * cast(cpu.gpr[data.rs]) +
                  (data.acc ? (cast(cpu.gpr[data.rdhi]) << 32) |
                                cast(cpu.gpr[data.rdlo])
                            : 0);

                cpu.gpr[data.rdlo] = bit_range(eval, 0, 31);
                cpu.gpr[data.rdhi] = bit_range(eval, 32, 63);

            } else {
                auto cast = [](uint32_t x) -> int64_t {
                    return static_cast<int64_t>(static_cast<int32_t>(x));
                };

                int64_t eval = cast(cpu.gpr[data.rm]) * cast(cpu.gpr[data.rs]) +
                               (data.acc ? (cast(cpu.gpr[data.rdhi]) << 32) |
                                             cast(cpu.gpr[data.rdlo])
                                         : 0);

                cpu.gpr[data.rdlo] = bit_range(eval, 0, 31);
                cpu.gpr[data.rdhi] = bit_range(eval, 32, 63);
            }

            if (data.set) {
                cpu.cpsr.set_z(cpu.gpr[data.rdhi] == 0 &&
                               cpu.gpr[data.rdlo] == 0);
                cpu.cpsr.set_n(get_bit(cpu.gpr[data.rdhi], 31));
                cpu.cpsr.set_c(0);
                cpu.cpsr.set_v(0);
            }
        },
        [](Undefined) { glogger.warn("Undefined instruction"); },
        [&cpu, pc_error](SingleDataSwap& data) {
            pc_error(data.rm);
            pc_error(data.rn);
            pc_error(data.rd);

            if (data.byte) {
                cpu.gpr[data.rd] = cpu.bus->read_byte(cpu.gpr[data.rn]);
                cpu.bus->write_byte(cpu.gpr[data.rn], cpu.gpr[data.rm] & 0xFF);
            } else {
                cpu.gpr[data.rd] = cpu.bus->read_word(cpu.gpr[data.rn]);
                cpu.bus->write_word(cpu.gpr[data.rn], cpu.gpr[data.rm]);
            }
        },
        [&cpu, pc_warn, pc_error](SingleDataTransfer& data) {
            uint32_t offset  = 0;
            uint32_t address = cpu.gpr[data.rn];

            if (!data.pre && data.write)
                glogger.warn("Write-back enabled with post-indexing in {}",
                             typeid(data).name());

            if (data.rn == cpu.PC_INDEX && data.write)
                glogger.warn("Write-back enabled with base register as PC {}",
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
                                         : cpu.gpr[shift->data.operand] & 0xFF);

                bool carry = cpu.cpsr.c();

                if (!shift->data.immediate)
                    pc_error(shift->data.operand);
                pc_error(shift->rm);

                offset = eval_shift(
                  shift->data.type, cpu.gpr[shift->rm], amount, carry);

                cpu.cpsr.set_c(carry);
            }

            // PC is always two instructions ahead
            if (data.rn == cpu.PC_INDEX)
                address -= 2 * INSTRUCTION_SIZE;

            if (data.pre)
                address += (data.up ? offset : -offset);

            // load
            if (data.load) {
                // byte
                if (data.byte)
                    cpu.gpr[data.rd] = cpu.bus->read_byte(address);
                // word
                else
                    cpu.gpr[data.rd] = cpu.bus->read_word(address);
                // store
            } else {
                // take PC into consideration
                if (data.rd == cpu.PC_INDEX)
                    address += INSTRUCTION_SIZE;

                // byte
                if (data.byte)
                    cpu.bus->write_byte(address, cpu.gpr[data.rd] & 0xFF);
                // word
                else
                    cpu.bus->write_word(address, cpu.gpr[data.rd]);
            }

            if (!data.pre)
                address += (data.up ? offset : -offset);

            if (!data.pre || data.write)
                cpu.gpr[data.rn] = address;

            if (data.rd == cpu.PC_INDEX && data.load)
                cpu.is_flushed = true;
        },
        [&cpu, pc_warn, pc_error](HalfwordTransfer& data) {
            uint32_t address = cpu.gpr[data.rn];
            uint32_t offset  = 0;

            if (!data.pre && data.write)
                glogger.error("Write-back enabled with post-indexing in {}",
                              typeid(data).name());

            if (data.sign && !data.load)
                glogger.error("Signed data found in {}", typeid(data).name());

            if (data.write)
                pc_warn(data.rn);

            // offset is register number (4 bits) when not an immediate
            if (!data.imm) {
                pc_error(data.offset);
                offset = cpu.gpr[data.offset];
            } else {
                offset = data.offset;
            }

            // PC is always two instructions ahead
            if (data.rn == cpu.PC_INDEX)
                address -= 2 * INSTRUCTION_SIZE;

            if (data.pre)
                address += (data.up ? offset : -offset);

            // load
            if (data.load) {
                // signed
                if (data.sign) {
                    // halfword
                    if (data.half) {
                        cpu.gpr[data.rd] = cpu.bus->read_halfword(address);

                        // sign extend the halfword
                        cpu.gpr[data.rd] =
                          (static_cast<int32_t>(cpu.gpr[data.rd]) << 16) >> 16;

                        // byte
                    } else {
                        cpu.gpr[data.rd] = cpu.bus->read_byte(address);

                        // sign extend the byte
                        cpu.gpr[data.rd] =
                          (static_cast<int32_t>(cpu.gpr[data.rd]) << 24) >> 24;
                    }
                    // unsigned halfword
                } else if (data.half) {
                    cpu.gpr[data.rd] = cpu.bus->read_halfword(address);
                }
                // store
            } else {
                // take PC into consideration
                if (data.rd == cpu.PC_INDEX)
                    address += INSTRUCTION_SIZE;

                // halfword
                if (data.half)
                    cpu.bus->write_halfword(address, cpu.gpr[data.rd]);
            }

            if (!data.pre)
                address += (data.up ? offset : -offset);

            if (!data.pre || data.write)
                cpu.gpr[data.rn] = address;

            if (data.rd == cpu.PC_INDEX && data.load)
                cpu.is_flushed = true;
        },
        [&cpu, pc_error](BlockDataTransfer& data) {
            uint32_t address  = cpu.gpr[data.rn];
            Mode mode         = cpu.cpsr.mode();
            uint8_t alignment = 4; // word
            uint8_t i         = 0;
            uint8_t n_regs    = std::popcount(data.regs);

            pc_error(data.rn);

            if (cpu.cpsr.mode() == Mode::User && data.s) {
                glogger.error("Bit S is set outside priviliged modes in {}",
                              typeid(data).name());
            }

            // we just change modes to load user registers
            if ((!get_bit(data.regs, cpu.PC_INDEX) && data.s) ||
                (!data.load && data.s)) {
                cpu.chg_mode(Mode::User);

                if (data.write) {
                    glogger.error(
                      "Write-back enable for user bank registers in {}",
                      typeid(data).name());
                }
            }

            // TODO: clean this shit
            // account for decrement
            if (!data.up)
                address -= (n_regs - 1) * alignment;

            if (data.pre)
                address += (data.up ? alignment : -alignment);

            if (data.load) {
                if (get_bit(data.regs, cpu.PC_INDEX) && data.s && data.load) {
                    // current mode's cpu.spsr is already loaded when it was
                    // switched
                    cpu.spsr = cpu.cpsr;
                }

                for (i = 0; i < cpu.GPR_COUNT; i++) {
                    if (get_bit(data.regs, i)) {
                        cpu.gpr[i] = cpu.bus->read_word(address);
                        address += alignment;
                    }
                }
            } else {
                for (i = 0; i < cpu.GPR_COUNT; i++) {
                    if (get_bit(data.regs, i)) {
                        cpu.bus->write_word(address, cpu.gpr[i]);
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
                cpu.gpr[data.rn] = address;

            if (data.load && get_bit(data.regs, cpu.PC_INDEX))
                cpu.is_flushed = true;

            // load back the original mode registers
            cpu.chg_mode(mode);
        },
        [&cpu, pc_error](PsrTransfer& data) {
            if (data.spsr && cpu.cpsr.mode() == Mode::User) {
                glogger.error("Accessing CPU.SPSR in User mode in {}",
                              typeid(data).name());
            }

            Psr& psr = data.spsr ? cpu.spsr : cpu.cpsr;

            switch (data.type) {
                case PsrTransfer::Type::Mrs:
                    pc_error(data.operand);
                    cpu.gpr[data.operand] = psr.raw();
                    break;
                case PsrTransfer::Type::Msr:
                    pc_error(data.operand);

                    if (cpu.cpsr.mode() != Mode::User) {
                        psr.set_all(cpu.gpr[data.operand]);
                    }
                    break;
                case PsrTransfer::Type::Msr_flg:
                    uint32_t operand =
                      (data.imm ? data.operand : cpu.gpr[data.operand]);
                    psr.set_n(get_bit(operand, 31));
                    psr.set_z(get_bit(operand, 30));
                    psr.set_c(get_bit(operand, 29));
                    psr.set_v(get_bit(operand, 28));
                    break;
            }
        },
        [&cpu, pc_error](DataProcessing& data) {
            using OpCode = DataProcessing::OpCode;

            uint32_t op_1 = cpu.gpr[data.rn];
            uint32_t op_2 = 0;

            uint32_t result = 0;

            if (const uint32_t* immediate =
                  std::get_if<uint32_t>(&data.operand)) {
                op_2 = *immediate;
            } else if (const Shift* shift = std::get_if<Shift>(&data.operand)) {
                uint8_t amount =
                  (shift->data.immediate ? shift->data.operand
                                         : cpu.gpr[shift->data.operand] & 0xFF);

                bool carry = cpu.cpsr.c();

                if (!shift->data.immediate)
                    pc_error(shift->data.operand);
                pc_error(shift->rm);

                op_2 = eval_shift(
                  shift->data.type, cpu.gpr[shift->rm], amount, carry);

                cpu.cpsr.set_c(carry);

                // PC is 12 bytes ahead when shifting
                if (data.rn == cpu.PC_INDEX)
                    op_1 += INSTRUCTION_SIZE;
            }

            bool overflow = cpu.cpsr.v();
            bool carry    = cpu.cpsr.c();

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
                    result = sub(op_1, op_2, carry, overflow);
                    break;
                case OpCode::RSB:
                    result = sub(op_2, op_1, carry, overflow);
                    break;
                case OpCode::ADD:
                case OpCode::CMN:
                    result = add(op_1, op_2, carry, overflow);
                    break;
                case OpCode::ADC:
                    result = add(op_1, op_2, carry, overflow, carry);
                    break;
                case OpCode::SBC:
                    result = sbc(op_1, op_2, carry, overflow, carry);
                    break;
                case OpCode::RSC:
                    result = sbc(op_2, op_1, carry, overflow, carry);
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

            auto set_conditions = [&cpu, carry, overflow, result]() {
                cpu.cpsr.set_c(carry);
                cpu.cpsr.set_v(overflow);
                cpu.cpsr.set_n(get_bit(result, 31));
                cpu.cpsr.set_z(result == 0);
            };

            if (data.set) {
                if (data.rd == cpu.PC_INDEX) {
                    if (cpu.cpsr.mode() == Mode::User)
                        glogger.error("Running {} in User mode",
                                      typeid(data).name());
                    cpu.spsr = cpu.cpsr;
                } else {
                    set_conditions();
                }
            }

            if (data.opcode == OpCode::TST || data.opcode == OpCode::TEQ ||
                data.opcode == OpCode::CMP || data.opcode == OpCode::CMN) {
                set_conditions();
            } else {
                cpu.gpr[data.rd] = result;
                if (data.rd == cpu.PC_INDEX || data.opcode == OpCode::MVN)
                    cpu.is_flushed = true;
            }
        },
        [&cpu](SoftwareInterrupt) {
            cpu.chg_mode(Mode::Supervisor);
            cpu.pc   = 0x08;
            cpu.spsr = cpu.cpsr;
        },
        [](auto& data) {
            glogger.error("Unimplemented {} instruction", typeid(data).name());
        } },
      data);
}
}
