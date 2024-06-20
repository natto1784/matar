#include "bus.hh"
#include "cpu/cpu.hh"
#include "util/bits.hh"
#include "util/log.hh"

namespace matar {
void
Cpu::exec(arm::Instruction& instruction) {
    bool is_flushed = false;

    if (!cpsr.condition(instruction.condition)) {
        advance_pc_arm();
        return;
    }

    auto pc_error = [](uint8_t r) {
        if (r == PC_INDEX)
            glogger.error("Using PC (R15) as operand register");
    };

    auto pc_warn = [](uint8_t r) {
        if (r == PC_INDEX)
            glogger.warn("Using PC (R15) as operand register");
    };

    using namespace arm;

    std::visit(
      overloaded{
        [this, pc_warn, &is_flushed](BranchAndExchange& data) {
            /*
              S -> reading instruction in step()
              N -> fetch from the new address in branch
              S -> last opcode fetch at +L to refill the pipeline
              Total = 2S + N cycles
                      1S done, S+N taken care of by flush_pipeline()
            */

            uint32_t addr = gpr[data.rn];
            State state   = static_cast<State>(get_bit(addr, 0));

            pc_warn(data.rn);

            if (state != cpsr.state())
                glogger.info_bold("State changed");

            // set state
            cpsr.set_state(state);

            // copy to PC
            pc = addr;

            // ignore [1:0] bits for arm and 0 bit for thumb
            rst_bit(pc, 0);

            if (state == State::Arm)
                rst_bit(pc, 1);

            // PC is affected so flush the pipeline
            is_flushed = true;
        },
        [this, &is_flushed](Branch& data) {
            /*
              S -> reading instruction in step()
              N -> fetch from the new address in branch
              S -> last opcode fetch at +L to refill the pipeline
              Total = 2S + N cycles
                      1S done, S+N taken care of by flush_pipeline()
            */

            if (data.link)
                gpr[14] = pc - INSTRUCTION_SIZE;

            pc += data.offset;

            // pc is affected so flush the pipeline
            is_flushed = true;
        },
        [this, pc_error](Multiply& data) {
            /*
              S -> reading instruction in step()
              mI -> m internal cycles
              I -> only when accumulating
              let v = data at rn
              m = 1 if bits [32:8] of v are all zero or all one
              m = 2         [32:16]
              m = 3         [32:24]
              m = 4         otherwise

              Total = S + mI or S + (m+1)I
            */

            if (data.rd == data.rm)
                glogger.error("rd and rm are not distinct in {}",
                              typeid(data).name());

            pc_error(data.rd);
            pc_error(data.rd);
            pc_error(data.rd);

            // mI
            for (int i = 0; i < multiplier_array_cycles(gpr[data.rs]); i++)
                internal_cycle();

            gpr[data.rd] = gpr[data.rm] * gpr[data.rs];

            if (data.acc) {
                gpr[data.rd] += gpr[data.rn];
                // 1I
                internal_cycle();
            }

            if (data.set) {
                cpsr.set_z(gpr[data.rd] == 0);
                cpsr.set_n(get_bit(gpr[data.rd], 31));
                cpsr.set_c(0);
            }
        },
        [this, pc_error](MultiplyLong& data) {
            /*
              S -> reading instruction in step()
              (m+1)I -> m + 1 internal cycles
              I -> only when accumulating
              let v = data at rs
              m = 1 if bits [32:8] of v are all zeroes (or all ones if signed)
              m = 2         [32:16]
              m = 3         [32:24]
              m = 4         otherwise

              Total = S + (m+1)I or S + (m+2)I
            */

            if (data.rdhi == data.rdlo || data.rdhi == data.rm ||
                data.rdlo == data.rm)
                glogger.error("rdhi, rdlo and rm are not distinct in {}",
                              typeid(data).name());

            pc_error(data.rdhi);
            pc_error(data.rdlo);
            pc_error(data.rm);
            pc_error(data.rs);

            // 1I
            if (data.acc)
                internal_cycle();

            // m+1 internal cycles
            for (int i = 0;
                 i <= multiplier_array_cycles(gpr[data.rs], data.uns);
                 i++)
                internal_cycle();

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
        [](Undefined) {
            // this should be 2S + N + I, should i flush the pipeline? i
            // dont know. TODO: study
            glogger.warn("Undefined instruction");
        },
        [this, pc_error](SingleDataSwap& data) {
            /*
              N -> reading instruction in step()
              N -> unrelated read
              S -> related write
              I -> earlier read value is written to register
              Total = S + 2N +I
            */

            pc_error(data.rm);
            pc_error(data.rn);
            pc_error(data.rd);

            if (data.byte) {
                gpr[data.rd] =
                  bus->read_byte(gpr[data.rn], CpuAccess::NonSequential);
                bus->write_byte(
                  gpr[data.rn], gpr[data.rm] & 0xFF, CpuAccess::Sequential);
            } else {
                gpr[data.rd] =
                  bus->read_word(gpr[data.rn], CpuAccess::NonSequential);
                bus->write_word(
                  gpr[data.rn], gpr[data.rm], CpuAccess::Sequential);
            }

            internal_cycle();
            // last write address is unrelated to next
            next_access = CpuAccess::NonSequential;
        },
        [this, pc_warn, pc_error, &is_flushed](SingleDataTransfer& data) {
            /*
              Load
              ====
              S   -> reading instruction in step()
              N   -> read from target
              I   -> stored in register
              N+S -> if PC is written - taken care of by flush_pipeline()
              Total = S + N + I or 2S + 2N + I

              Store
              =====
              N -> calculating memory address
              N -> write at target
              Total = 2N
            */
            uint32_t offset  = 0;
            uint32_t address = gpr[data.rn];

            if (!data.pre && data.write)
                glogger.warn("Write-back enabled with post-indexing in {}",
                             typeid(data).name());

            if (data.rn == PC_INDEX && data.write)
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
                                         : gpr[shift->data.operand] & 0xFF);

                bool carry = cpsr.c();

                if (!shift->data.immediate)
                    pc_error(shift->data.operand);
                pc_error(shift->rm);

                offset =
                  eval_shift(shift->data.type, gpr[shift->rm], amount, carry);

                cpsr.set_c(carry);
            }

            if (data.pre)
                address += (data.up ? offset : -offset);

            // load
            if (data.load) {
                // byte
                if (data.byte)
                    gpr[data.rd] =
                      bus->read_byte(address, CpuAccess::NonSequential);
                // word
                else
                    gpr[data.rd] =
                      bus->read_word(address, CpuAccess::NonSequential);

                // N + S
                if (data.rd == PC_INDEX)
                    is_flushed = true;

                // I
                internal_cycle();
                // store
            } else {
                // take PC into consideration
                uint32_t value = gpr[data.rd];

                if (data.rd == PC_INDEX)
                    value += INSTRUCTION_SIZE;

                // byte
                if (data.byte)
                    bus->write_byte(
                      address, value & 0xFF, CpuAccess::NonSequential);
                // word
                else
                    bus->write_word(address, value, CpuAccess::NonSequential);
            }

            if (!data.pre)
                address += (data.up ? offset : -offset);

            if (!data.pre || data.write)
                gpr[data.rn] = address;

            // last read/write is unrelated, this will be overwriten if
            // flushed
            next_access = CpuAccess::NonSequential;
        },
        [this, pc_warn, pc_error, &is_flushed](HalfwordTransfer& data) {
            /*
              Load
              ====
              S   -> reading instruction in step()
              N   -> read from target
              I   -> stored in register
              N+S -> if PC is written - taken care of by flush_pipeline()
              Total = S + N + I or 2S + 2N + I

              Store
              =====
              N -> calculating memory address
              N -> write at target
              Total = 2N
            */
            uint32_t address = gpr[data.rn];
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
                offset = gpr[data.offset];
            } else {
                offset = data.offset;
            }

            if (data.pre)
                address += (data.up ? offset : -offset);

            // load
            if (data.load) {
                // signed
                if (data.sign) {
                    // halfword
                    if (data.half) {
                        gpr[data.rd] =
                          bus->read_halfword(address, CpuAccess::NonSequential);

                        // sign extend the halfword
                        gpr[data.rd] =
                          (static_cast<int32_t>(gpr[data.rd]) << 16) >> 16;

                        // byte
                    } else {
                        gpr[data.rd] =
                          bus->read_byte(address, CpuAccess::NonSequential);

                        // sign extend the byte
                        gpr[data.rd] =
                          (static_cast<int32_t>(gpr[data.rd]) << 24) >> 24;
                    }
                    // unsigned halfword
                } else if (data.half) {
                    gpr[data.rd] =
                      bus->read_halfword(address, CpuAccess::NonSequential);
                }

                // I
                internal_cycle();

                if (data.rd == PC_INDEX)
                    is_flushed = true;

                // store
            } else {
                uint32_t value = gpr[data.rd];

                // take PC into consideration
                if (data.rd == PC_INDEX)
                    value += INSTRUCTION_SIZE;

                // halfword
                if (data.half)
                    bus->write_halfword(
                      address, value & 0xFFFF, CpuAccess::NonSequential);
            }

            if (!data.pre)
                address += (data.up ? offset : -offset);

            if (!data.pre || data.write)
                gpr[data.rn] = address;

            // last read/write is unrelated, this will be overwriten if
            // flushed
            next_access = CpuAccess::NonSequential;
        },
        [this, pc_error, &is_flushed](BlockDataTransfer& data) {
            /*
              Load
              ====
              S       -> reading instruction in step()
              N       -> unrelated read from target
              (n-1) S -> next n - 1 related reads from target
              I       -> stored in register
              N+S     -> if PC is written - taken care of by
              flush_pipeline() Total = nS + N + I or (n+1)S + 2N + I

              Store
              =====
              N       -> calculating memory address
              N       -> unrelated write at target
              (n-1) S -> next n - 1 related writes
              Total = 2N + (n-1)S
            */

            static constexpr uint8_t alignment = 4; // word

            uint32_t address = gpr[data.rn];
            Mode mode        = cpsr.mode();
            int8_t i         = 0;
            CpuAccess access = CpuAccess::NonSequential;

            pc_error(data.rn);

            if (cpsr.mode() == Mode::User && data.s) {
                glogger.error("Bit S is set outside priviliged modes in block "
                              "data transfer");
            }

            // we just change modes to load user registers
            if ((!get_bit(data.regs, PC_INDEX) && data.s) ||
                (!data.load && data.s)) {
                chg_mode(Mode::User);

                if (data.write) {
                    glogger.error("Write-back enable for user bank registers "
                                  "in block data transfer");
                }
            }

            // increment beforehand
            if (data.pre)
                address += (data.up ? alignment : -alignment);

            if (data.load) {
                if (get_bit(data.regs, PC_INDEX)) {
                    is_flushed = true;

                    // current mode's spsr is already loaded when it was
                    // switched
                    if (data.s)
                        spsr = cpsr;
                }

                if (data.up) {
                    for (i = 0; i < GPR_COUNT; i++) {
                        if (get_bit(data.regs, i)) {
                            gpr[i] = bus->read_word(address, access);
                            address += alignment;
                            access = CpuAccess::Sequential;
                        }
                    }
                } else {
                    for (i = GPR_COUNT - 1; i >= 0; i--) {
                        if (get_bit(data.regs, i)) {
                            gpr[i] = bus->read_word(address, access);
                            address -= alignment;
                            access = CpuAccess::Sequential;
                        }
                    }
                }

                // I
                internal_cycle();
            } else {
                if (data.up) {
                    for (i = 0; i < GPR_COUNT; i++) {
                        if (get_bit(data.regs, i)) {
                            bus->write_word(address, gpr[i], access);
                            address += alignment;
                            access = CpuAccess::Sequential;
                        }
                    }
                } else {
                    for (i = GPR_COUNT - 1; i >= 0; i--) {
                        if (get_bit(data.regs, i)) {
                            bus->write_word(address, gpr[i], access);
                            address -= alignment;
                            access = CpuAccess::Sequential;
                        }
                    }
                }
            }

            // fix increment
            if (data.pre)
                address += (data.up ? -alignment : alignment);

            if (!data.pre || data.write)
                gpr[data.rn] = address;

            // load back the original mode registers
            chg_mode(mode);

            // last read/write is unrelated, this will be overwriten if
            // flushed
            next_access = CpuAccess::NonSequential;
        },
        [this, pc_error](PsrTransfer& data) {
            /*
              S -> prefetched instruction in step()
              Total = 1S cycle
            */

            if (data.spsr && cpsr.mode() == Mode::User) {
                glogger.error("Accessing SPSR in User mode in {}",
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
                        if (!data.spsr) {
                            Psr tmp = Psr(gpr[data.operand]);
                            chg_mode(tmp.mode());
                        }

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
        [this, pc_error, &is_flushed](DataProcessing& data) {
            /*
              Always
              ======
              S -> prefetched instruction in step()

              With Register specified shift
              =============================
              I -> internal cycle

              When PC is written
              ==================
              N -> fetch from the new address in branch
              S -> last opcode fetch at +L to refill the pipeline
              S+N taken care of by flush_pipeline()

              Total = S or S + I or 2S + N + I or 2S + N cycles
            */

            using OpCode = DataProcessing::OpCode;

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

                // 1I when register specified shift
                if (!shift->data.immediate)
                    internal_cycle();
            }

            bool overflow = cpsr.v();
            bool carry    = cpsr.c();

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

            auto set_conditions = [this, carry, overflow, result]() {
                cpsr.set_c(carry);
                cpsr.set_v(overflow);
                cpsr.set_n(get_bit(result, 31));
                cpsr.set_z(result == 0);
            };

            if (data.set) {
                if (data.rd == PC_INDEX) {
                    if (cpsr.mode() == Mode::User)
                        glogger.error("Running {} in User mode",
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
        [this, &is_flushed](SoftwareInterrupt) {
            chg_mode(Mode::Supervisor);
            pc         = 0x00;
            spsr       = cpsr;
            is_flushed = true;
        },
        [](auto& data) {
            glogger.error("Unimplemented {} instruction", typeid(data).name());
        } },
      instruction.data);

    if (is_flushed) {
        opcodes[0] = bus->read_word(pc, CpuAccess::NonSequential);
        advance_pc_arm();
        opcodes[1] = bus->read_word(pc, CpuAccess::Sequential);
        advance_pc_arm();
        next_access = CpuAccess::Sequential;
    } else
        advance_pc_arm();
}
}
