#include "cpu/alu.hh"
#include "cpu/cpu.hh"
#include "util/bits.hh"
#include "util/log.hh"

namespace matar::thumb {
void
Instruction::exec(Cpu& cpu) {
    auto set_cc = [&cpu](bool c, bool v, bool n, bool z) {
        cpu.cpsr.set_c(c);
        cpu.cpsr.set_v(v);
        cpu.cpsr.set_n(n);
        cpu.cpsr.set_z(z);
    };

    std::visit(
      overloaded{
        [&cpu, set_cc](MoveShiftedRegister& data) {
            /*
              S -> prefetched instruction in step()

              Total = S cycle
            */
            if (data.opcode == ShiftType::ROR)
                glogger.error("Invalid opcode in {}", typeid(data).name());

            bool carry = cpu.cpsr.c();

            uint32_t shifted =
              eval_shift(data.opcode, cpu.gpr[data.rs], data.offset, carry);

            cpu.gpr[data.rd] = shifted;

            set_cc(carry, cpu.cpsr.v(), get_bit(shifted, 31), shifted == 0);
        },
        [&cpu, set_cc](AddSubtract& data) {
            /*
              S -> prefetched instruction in step()

              Total = S cycle
            */
            uint32_t offset =
              data.imm ? static_cast<uint32_t>(static_cast<int8_t>(data.offset))
                       : cpu.gpr[data.offset];
            uint32_t result = 0;
            bool carry      = cpu.cpsr.c();
            bool overflow   = cpu.cpsr.v();

            switch (data.opcode) {
                case AddSubtract::OpCode::ADD:
                    result = add(cpu.gpr[data.rs], offset, carry, overflow);
                    break;
                case AddSubtract::OpCode::SUB:
                    result = sub(cpu.gpr[data.rs], offset, carry, overflow);
                    break;
            }

            cpu.gpr[data.rd] = result;
            set_cc(carry, overflow, get_bit(result, 31), result == 0);
        },
        [&cpu, set_cc](MovCmpAddSubImmediate& data) {
            /*
              S -> prefetched instruction in step()

              Total = S cycle
            */
            uint32_t result = 0;
            bool carry      = cpu.cpsr.c();
            bool overflow   = cpu.cpsr.v();

            switch (data.opcode) {
                case MovCmpAddSubImmediate::OpCode::MOV:
                    result = data.offset;
                    carry  = 0;
                    break;
                case MovCmpAddSubImmediate::OpCode::ADD:
                    result =
                      add(cpu.gpr[data.rd], data.offset, carry, overflow);
                    break;
                case MovCmpAddSubImmediate::OpCode::SUB:
                case MovCmpAddSubImmediate::OpCode::CMP:
                    result =
                      sub(cpu.gpr[data.rd], data.offset, carry, overflow);
                    break;
            }

            set_cc(carry, overflow, get_bit(result, 31), result == 0);
            if (data.opcode != MovCmpAddSubImmediate::OpCode::CMP)
                cpu.gpr[data.rd] = result;
        },
        [&cpu, set_cc](AluOperations& data) {
            /*
              Data Processing
              ===============
              S -> prefetched instruction in step()
              I -> only when register specified shift
              Total = S or S + I cycles

              Multiply
              ========
              S -> reading instruction in step()
              mI -> m internal cycles
              let v = data at rn
              m = 1 if bits [32:8] of v are all zero or all one
              m = 2         [32:16]
              m = 3         [32:24]
              m = 4         otherwise

              Total = S + mI cycles
            */
            uint32_t op_1   = cpu.gpr[data.rd];
            uint32_t op_2   = cpu.gpr[data.rs];
            uint32_t result = 0;

            bool carry    = cpu.cpsr.c();
            bool overflow = cpu.cpsr.v();

            switch (data.opcode) {
                case AluOperations::OpCode::AND:
                case AluOperations::OpCode::TST:
                    result = op_1 & op_2;
                    break;
                case AluOperations::OpCode::EOR:
                    result = op_1 ^ op_2;
                    break;
                case AluOperations::OpCode::LSL:
                    result = eval_shift(ShiftType::LSL, op_1, op_2, carry);
                    cpu.internal_cycle();
                    break;
                case AluOperations::OpCode::LSR:
                    result = eval_shift(ShiftType::LSR, op_1, op_2, carry);
                    cpu.internal_cycle();
                    break;
                case AluOperations::OpCode::ASR:
                    result = eval_shift(ShiftType::ASR, op_1, op_2, carry);
                    cpu.internal_cycle();
                    break;
                case AluOperations::OpCode::ADC:
                    result = add(op_1, op_2, carry, overflow, carry);
                    break;
                case AluOperations::OpCode::SBC:
                    result = sbc(op_1, op_2, carry, overflow, carry);
                    break;
                case AluOperations::OpCode::ROR:
                    result = eval_shift(ShiftType::ROR, op_1, op_2, carry);
                    cpu.internal_cycle();
                    break;
                case AluOperations::OpCode::NEG:
                    result = -op_2;
                    break;
                case AluOperations::OpCode::CMP:
                    result = sub(op_1, op_2, carry, overflow);
                    break;
                case AluOperations::OpCode::CMN:
                    result = add(op_1, op_2, carry, overflow);
                    break;
                case AluOperations::OpCode::ORR:
                    result = op_1 | op_2;
                    break;
                case AluOperations::OpCode::MUL:
                    result = op_1 * op_2;
                    // mI cycles
                    for (int i = 0; i < multiplier_array_cycles(op_2); i++)
                        cpu.internal_cycle();
                    break;
                case AluOperations::OpCode::BIC:
                    result = op_1 & ~op_2;
                    break;
                case AluOperations::OpCode::MVN:
                    result = ~op_2;
                    break;
            }

            if (data.opcode != AluOperations::OpCode::TST &&
                data.opcode != AluOperations::OpCode::CMP &&
                data.opcode != AluOperations::OpCode::CMN)
                cpu.gpr[data.rd] = result;

            set_cc(carry, overflow, get_bit(result, 31), result == 0);
        },
        [&cpu, set_cc](HiRegisterOperations& data) {
            /*
              Always
              ======
              S -> prefetched instruction in step()

              When PC is written
              ==================
              N -> fetch from the new address in branch
              S -> last opcode fetch at +L to refill the pipeline
              S+N taken care of by flush_pipeline()

              Total = S or 2S + N cycles
            */

            uint32_t op_1 = cpu.gpr[data.rd];
            uint32_t op_2 = cpu.gpr[data.rs];

            bool carry    = cpu.cpsr.c();
            bool overflow = cpu.cpsr.v();

            // PC is already current + 4, so dont need to do that
            if (data.rd == cpu.PC_INDEX)
                rst_bit(op_1, 0);

            if (data.rs == cpu.PC_INDEX)
                rst_bit(op_2, 0);

            switch (data.opcode) {
                case HiRegisterOperations::OpCode::ADD: {
                    cpu.gpr[data.rd] = add(op_1, op_2, carry, overflow);

                    if (data.rd == cpu.PC_INDEX)
                        cpu.is_flushed = true;
                } break;
                case HiRegisterOperations::OpCode::CMP: {
                    uint32_t result = sub(op_1, op_2, carry, overflow);
                    set_cc(carry, overflow, get_bit(result, 31), result == 0);
                } break;
                case HiRegisterOperations::OpCode::MOV: {
                    cpu.gpr[data.rd] = op_2;

                    if (data.rd == cpu.PC_INDEX)
                        cpu.is_flushed = true;
                } break;
                case HiRegisterOperations::OpCode::BX: {
                    State state = static_cast<State>(get_bit(op_2, 0));

                    if (state != cpu.cpsr.state())
                        glogger.info_bold("State changed");

                    // set state
                    cpu.cpsr.set_state(state);

                    // copy to PC
                    cpu.pc = op_2;

                    // ignore [1:0] bits for arm and 0 bit for thumb
                    rst_bit(cpu.pc, 0);

                    if (state == State::Arm)
                        rst_bit(cpu.pc, 1);

                    // pc is affected so flush the pipeline
                    cpu.is_flushed = true;
                } break;
            }
        },
        [&cpu](PcRelativeLoad& data) {
            /*
              S   -> reading instruction in step()
              N   -> read from target
              I   -> stored in register
              Total = S + N + I cycles
            */
            uint32_t pc = cpu.pc;
            rst_bit(pc, 0);
            rst_bit(pc, 1);

            cpu.gpr[data.rd] = cpu.bus->read_word(pc + data.word, false);

            cpu.internal_cycle();

            // last read is unrelated
            cpu.sequential = false;
        },
        [&cpu](LoadStoreRegisterOffset& data) {
            /*
              Load
              ====
              S   -> reading instruction in step()
              N   -> read from target
              I   -> stored in register
              Total = S + N + I

              Store
              =====
              N -> calculating memory address
              N -> write at target
              Total = 2N
            */

            uint32_t address = cpu.gpr[data.rb] + cpu.gpr[data.ro];

            if (data.load) {
                if (data.byte) {
                    cpu.gpr[data.rd] = cpu.bus->read_byte(address, false);
                } else {
                    cpu.gpr[data.rd] = cpu.bus->read_word(address, false);
                }
                cpu.internal_cycle();
            } else {
                if (data.byte) {
                    cpu.bus->write_byte(
                      address, cpu.gpr[data.rd] & 0xFF, false);
                } else {
                    cpu.bus->write_word(address, cpu.gpr[data.rd], false);
                }
            }

            // last read/write is unrelated
            cpu.sequential = false;
        },
        [&cpu](LoadStoreSignExtendedHalfword& data) {
            // Same cycles as above

            uint32_t address = cpu.gpr[data.rb] + cpu.gpr[data.ro];

            switch (data.s << 1 | data.h) {
                case 0b00:
                    cpu.bus->write_halfword(
                      address, cpu.gpr[data.rd] & 0xFFFF, false);
                    break;
                case 0b01:
                    cpu.gpr[data.rd] = cpu.bus->read_halfword(address, false);
                    cpu.internal_cycle();
                    break;
                case 0b10:
                    // sign extend and load the byte
                    cpu.gpr[data.rd] =
                      (static_cast<int32_t>(cpu.bus->read_byte(address, false))
                       << 24) >>
                      24;
                    cpu.internal_cycle();
                    break;
                case 0b11:
                    // sign extend the halfword
                    cpu.gpr[data.rd] =
                      (static_cast<int32_t>(
                         cpu.bus->read_halfword(address, false))
                       << 16) >>
                      16;
                    cpu.internal_cycle();
                    break;

                    // unreachable
                default: {
                }
            }

            // last read/write is unrelated
            cpu.sequential = false;
        },
        [&cpu](LoadStoreImmediateOffset& data) {
            // Same cycles as above

            uint32_t address = cpu.gpr[data.rb] + data.offset;

            if (data.load) {
                if (data.byte) {
                    cpu.gpr[data.rd] = cpu.bus->read_byte(address, false);
                } else {
                    cpu.gpr[data.rd] = cpu.bus->read_word(address, false);
                }
                cpu.internal_cycle();
            } else {
                if (data.byte) {
                    cpu.bus->write_byte(
                      address, cpu.gpr[data.rd] & 0xFF, false);
                } else {
                    cpu.bus->write_word(address, cpu.gpr[data.rd], false);
                }
            }

            // last read/write is unrelated
            cpu.sequential = false;
        },
        [&cpu](LoadStoreHalfword& data) {
            // Same cycles as above

            uint32_t address = cpu.gpr[data.rb] + data.offset;

            if (data.load) {
                cpu.gpr[data.rd] = cpu.bus->read_halfword(address);
                cpu.internal_cycle();
            } else {
                cpu.bus->write_halfword(address, cpu.gpr[data.rd] & 0xFFFF);
            }

            // last read/write is unrelated
            cpu.sequential = false;
        },
        [&cpu](SpRelativeLoad& data) {
            // Same cycles as above

            uint32_t address = cpu.sp + data.word;

            if (data.load) {
                cpu.gpr[data.rd] = cpu.bus->read_word(address);
                cpu.internal_cycle();
            } else {
                cpu.bus->write_word(address, cpu.gpr[data.rd]);
            }

            // last read/write is unrelated
            cpu.sequential = false;
        },
        [&cpu](LoadAddress& data) {
            // 1S cycle in step()

            if (data.sp) {
                cpu.gpr[data.rd] = cpu.sp + data.word;
            } else {
                // PC is already current + 4, so dont need to do that
                // force bit 1 to 0
                cpu.gpr[data.rd] = (cpu.pc & ~(1 << 1)) + data.word;
            }
        },
        [&cpu](AddOffsetStackPointer& data) {
            // 1S cycle in step()

            cpu.sp += data.word;
        },
        [&cpu](PushPopRegister& data) {
            /*
              Load
              ====
              S       -> reading instruction in step()
              N       -> unrelated read from target
              (n-1) S -> next n - 1 related reads from target
              I       -> stored in register
              N+S     -> if PC is written - taken care of by flush_pipeline()
              Total = nS + N + I or (n+1)S + 2N + I

              Store
              =====
              N       -> calculating memory address
              N       -> unrelated write at target
              (n-1) S -> next n - 1 related writes
              Total = 2N + (n-1)S
            */
            static constexpr uint8_t alignment = 4;
            bool sequential                    = false;

            if (data.load) {
                for (uint8_t i = 0; i < 8; i++) {
                    if (get_bit(data.regs, i)) {
                        cpu.gpr[i] = cpu.bus->read_word(cpu.sp, sequential);
                        cpu.sp += alignment;
                        sequential = true;
                    }
                }

                if (data.pclr) {
                    cpu.pc = cpu.bus->read_word(cpu.sp);
                    cpu.sp += alignment;
                    cpu.is_flushed = true;
                }

                // I
                cpu.internal_cycle();
            } else {
                if (data.pclr) {
                    cpu.sp -= alignment;
                    cpu.bus->write_word(cpu.sp, cpu.lr);
                }

                for (int8_t i = 7; i >= 0; i--) {
                    if (get_bit(data.regs, i)) {
                        cpu.sp -= alignment;
                        cpu.bus->write_word(cpu.sp, cpu.gpr[i], sequential);
                        sequential = true;
                    }
                }
            }

            // last read/write is unrelated
            cpu.sequential = false;
        },
        [&cpu](MultipleLoad& data) {
            /*
              Load
              ====
              S       -> reading instruction in step()
              N       -> unrelated read from target
              (n-1) S -> next n - 1 related reads from target
              I       -> stored in register
              Total = nS + N + I

              Store
              =====
              N       -> calculating memory address
              N       -> unrelated write at target
              (n-1) S -> next n - 1 related writes
              Total = 2N + (n-1)S
            */

            static constexpr uint8_t alignment = 4;

            uint32_t rb     = cpu.gpr[data.rb];
            bool sequential = false;

            if (data.load) {
                for (uint8_t i = 0; i < 8; i++) {
                    if (get_bit(data.regs, i)) {
                        cpu.gpr[i] = cpu.bus->read_word(rb, sequential);
                        rb += alignment;
                        sequential = true;
                    }
                }
            } else {
                for (int8_t i = 7; i >= 0; i--) {
                    if (get_bit(data.regs, i)) {
                        rb -= alignment;
                        cpu.bus->write_word(rb, cpu.gpr[i], sequential);
                        sequential = true;
                    }
                }
            }

            cpu.gpr[data.rb] = rb;

            // last read/write is unrelated
            cpu.sequential = false;
        },
        [&cpu](ConditionalBranch& data) {
            /*
              S   -> reading instruction in step()
              N+S -> if condition is true, branch and refill pipeline
              Total = S or 2S + N
            */

            if (data.condition == Condition::AL)
                glogger.warn("Condition 1110 (AL) is undefined");

            if (!cpu.cpsr.condition(data.condition))
                return;

            cpu.pc += data.offset;
            cpu.is_flushed = true;
        },
        [&cpu](SoftwareInterrupt& data) {
            /*
              S   -> reading instruction in step()
              N+S -> refill pipeline
              Total = 2S + N
            */

            // next instruction is one instruction behind PC
            cpu.lr   = cpu.pc - INSTRUCTION_SIZE;
            cpu.spsr = cpu.cpsr;
            cpu.pc   = data.vector;
            cpu.cpsr.set_state(State::Arm);
            cpu.chg_mode(Mode::Supervisor);
            cpu.is_flushed = true;
        },
        [&cpu](UnconditionalBranch& data) {
            /*
              S   -> reading instruction in step()
              N+S -> branch and refill pipeline
              Total = 2S + N
            */

            cpu.pc += data.offset;
            cpu.is_flushed = true;
        },
        [&cpu](LongBranchWithLink& data) {
            /*
              S -> prefetched instruction in step()
              N -> fetch from the new address in branch
              S -> last opcode fetch at +L to refill the pipeline
              Total = 2S + N cycles
                      1S done, S+N taken care of by flush_pipeline()
            */

            // 12 bit integer
            int32_t offset = data.offset;

            if (data.high) {
                uint32_t old_pc = cpu.pc;

                cpu.pc         = cpu.lr + offset;
                cpu.lr         = (old_pc - INSTRUCTION_SIZE) | 1;
                cpu.is_flushed = true;
            } else {
                // 12 + 11 = 23 bit
                offset <<= 11;
                // sign extend
                offset = (offset << 9) >> 9;
                cpu.lr = cpu.pc + offset;
            }
        },
        [](auto& data) {
            glogger.error("Unknown thumb format : {}", typeid(data).name());
        } },
      data);
}
}
