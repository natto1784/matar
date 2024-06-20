#include "bus.hh"
#include "cpu/alu.hh"
#include "cpu/cpu.hh"
#include "util/bits.hh"
#include "util/log.hh"

namespace matar {
void
Cpu::exec(thumb::Instruction& instruction) {
    bool is_flushed = false;
    dbg(pc);

    auto set_cc = [this](bool c, bool v, bool n, bool z) {
        cpsr.set_c(c);
        cpsr.set_v(v);
        cpsr.set_n(n);
        cpsr.set_z(z);
    };

    using namespace thumb;

    std::visit(
      overloaded{
        [this, set_cc](MoveShiftedRegister& data) {
            /*
              S -> prefetched instruction in step()

              Total = S cycle
            */
            if (data.opcode == ShiftType::ROR)
                glogger.error("Invalid opcode in {}", typeid(data).name());

            bool carry = cpsr.c();

            uint32_t shifted =
              eval_shift(data.opcode, gpr[data.rs], data.offset, carry);

            gpr[data.rd] = shifted;

            set_cc(carry, cpsr.v(), get_bit(shifted, 31), shifted == 0);
        },
        [this, set_cc](AddSubtract& data) {
            /*
              S -> prefetched instruction in step()

              Total = S cycle
            */
            uint32_t offset =
              data.imm ? static_cast<uint32_t>(static_cast<int8_t>(data.offset))
                       : gpr[data.offset];
            uint32_t result = 0;
            bool carry      = cpsr.c();
            bool overflow   = cpsr.v();

            switch (data.opcode) {
                case AddSubtract::OpCode::ADD:
                    result = add(gpr[data.rs], offset, carry, overflow);
                    break;
                case AddSubtract::OpCode::SUB:
                    result = sub(gpr[data.rs], offset, carry, overflow);
                    break;
            }

            gpr[data.rd] = result;
            set_cc(carry, overflow, get_bit(result, 31), result == 0);
        },
        [this, set_cc](MovCmpAddSubImmediate& data) {
            /*
              S -> prefetched instruction in step()

              Total = S cycle
            */
            uint32_t result = 0;
            bool carry      = cpsr.c();
            bool overflow   = cpsr.v();

            switch (data.opcode) {
                case MovCmpAddSubImmediate::OpCode::MOV:
                    result = data.offset;
                    carry  = 0;
                    break;
                case MovCmpAddSubImmediate::OpCode::ADD:
                    result = add(gpr[data.rd], data.offset, carry, overflow);
                    break;
                case MovCmpAddSubImmediate::OpCode::SUB:
                case MovCmpAddSubImmediate::OpCode::CMP:
                    result = sub(gpr[data.rd], data.offset, carry, overflow);
                    break;
            }

            set_cc(carry, overflow, get_bit(result, 31), result == 0);
            if (data.opcode != MovCmpAddSubImmediate::OpCode::CMP)
                gpr[data.rd] = result;
        },
        [this, set_cc](AluOperations& data) {
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
            uint32_t op_1   = gpr[data.rd];
            uint32_t op_2   = gpr[data.rs];
            uint32_t result = 0;

            bool carry    = cpsr.c();
            bool overflow = cpsr.v();

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
                    internal_cycle();
                    break;
                case AluOperations::OpCode::LSR:
                    result = eval_shift(ShiftType::LSR, op_1, op_2, carry);
                    internal_cycle();
                    break;
                case AluOperations::OpCode::ASR:
                    result = eval_shift(ShiftType::ASR, op_1, op_2, carry);
                    internal_cycle();
                    break;
                case AluOperations::OpCode::ADC:
                    result = add(op_1, op_2, carry, overflow, carry);
                    break;
                case AluOperations::OpCode::SBC:
                    result = sbc(op_1, op_2, carry, overflow, carry);
                    break;
                case AluOperations::OpCode::ROR:
                    result = eval_shift(ShiftType::ROR, op_1, op_2, carry);
                    internal_cycle();
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
                        internal_cycle();
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
                gpr[data.rd] = result;

            set_cc(carry, overflow, get_bit(result, 31), result == 0);
        },
        [this, set_cc, &is_flushed](HiRegisterOperations& data) {
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

            uint32_t op_1 = gpr[data.rd];
            uint32_t op_2 = gpr[data.rs];

            bool carry    = cpsr.c();
            bool overflow = cpsr.v();

            // PC is already current + 4, so dont need to do that
            if (data.rd == PC_INDEX)
                rst_bit(op_1, 0);

            if (data.rs == PC_INDEX)
                rst_bit(op_2, 0);

            switch (data.opcode) {
                case HiRegisterOperations::OpCode::ADD: {
                    gpr[data.rd] = add(op_1, op_2, carry, overflow);

                    if (data.rd == PC_INDEX)
                        is_flushed = true;
                } break;
                case HiRegisterOperations::OpCode::CMP: {
                    uint32_t result = sub(op_1, op_2, carry, overflow);
                    set_cc(carry, overflow, get_bit(result, 31), result == 0);
                } break;
                case HiRegisterOperations::OpCode::MOV: {
                    gpr[data.rd] = op_2;

                    if (data.rd == PC_INDEX)
                        is_flushed = true;
                } break;
                case HiRegisterOperations::OpCode::BX: {
                    State state = static_cast<State>(get_bit(op_2, 0));

                    if (state != cpsr.state())
                        glogger.info_bold("State changed");

                    // set state
                    cpsr.set_state(state);

                    // copy to PC
                    pc = op_2;

                    // ignore [1:0] bits for arm and 0 bit for thumb
                    rst_bit(pc, 0);

                    if (state == State::Arm)
                        rst_bit(pc, 1);

                    // pc is affected so flush the pipeline
                    is_flushed = true;
                } break;
            }
        },
        [this](PcRelativeLoad& data) {
            /*
              S   -> reading instruction in step()
              N   -> read from target
              I   -> stored in register
              Total = S + N + I cycles
            */
            uint32_t pc_ = pc;
            rst_bit(pc_, 0);
            rst_bit(pc_, 1);

            gpr[data.rd] =
              bus->read_word(pc_ + data.word, CpuAccess::NonSequential);

            internal_cycle();

            // last read is unrelated
            next_access = CpuAccess::NonSequential;
        },
        [this](LoadStoreRegisterOffset& data) {
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

            uint32_t address = gpr[data.rb] + gpr[data.ro];

            if (data.load) {
                if (data.byte) {
                    gpr[data.rd] =
                      bus->read_byte(address, CpuAccess::NonSequential);
                } else {
                    gpr[data.rd] =
                      bus->read_word(address, CpuAccess::NonSequential);
                }
                internal_cycle();
            } else {
                if (data.byte) {
                    bus->write_byte(
                      address, gpr[data.rd] & 0xFF, CpuAccess::NonSequential);
                } else {
                    bus->write_word(
                      address, gpr[data.rd], CpuAccess::NonSequential);
                }
            }

            // last read/write is unrelated
            next_access = CpuAccess::NonSequential;
        },
        [this](LoadStoreSignExtendedHalfword& data) {
            // Same cycles as above

            uint32_t address = gpr[data.rb] + gpr[data.ro];

            switch (data.s << 1 | data.h) {
                case 0b00:
                    bus->write_halfword(
                      address, gpr[data.rd] & 0xFFFF, CpuAccess::NonSequential);
                    break;
                case 0b01:
                    gpr[data.rd] =
                      bus->read_halfword(address, CpuAccess::NonSequential);
                    internal_cycle();
                    break;
                case 0b10:
                    // sign extend and load the byte
                    gpr[data.rd] = (static_cast<int32_t>(bus->read_byte(
                                      address, CpuAccess::NonSequential))
                                    << 24) >>
                                   24;
                    internal_cycle();
                    break;
                case 0b11:
                    // sign extend the halfword
                    gpr[data.rd] = (static_cast<int32_t>(bus->read_halfword(
                                      address, CpuAccess::NonSequential))
                                    << 16) >>
                                   16;
                    internal_cycle();
                    break;

                    // unreachable
                default: {
                }
            }

            // last read/write is unrelated
            next_access = CpuAccess::NonSequential;
        },
        [this](LoadStoreImmediateOffset& data) {
            // Same cycles as above

            uint32_t address = gpr[data.rb] + data.offset;
            dbg(address);

            if (data.load) {
                if (data.byte) {
                    gpr[data.rd] =
                      bus->read_byte(address, CpuAccess::NonSequential);
                } else {
                    gpr[data.rd] =
                      bus->read_word(address, CpuAccess::NonSequential);
                }
                internal_cycle();
            } else {
                if (data.byte) {
                    bus->write_byte(
                      address, gpr[data.rd] & 0xFF, CpuAccess::NonSequential);
                } else {
                    bus->write_word(
                      address, gpr[data.rd], CpuAccess::NonSequential);
                }
            }

            // last read/write is unrelated
            next_access = CpuAccess::NonSequential;
        },
        [this](LoadStoreHalfword& data) {
            // Same cycles as above

            uint32_t address = gpr[data.rb] + data.offset;

            if (data.load) {
                gpr[data.rd] =
                  bus->read_halfword(address, CpuAccess::NonSequential);
                internal_cycle();
            } else {
                bus->write_halfword(
                  address, gpr[data.rd] & 0xFFFF, CpuAccess::NonSequential);
            }

            // last read/write is unrelated
            next_access = CpuAccess::NonSequential;
        },
        [this](SpRelativeLoad& data) {
            // Same cycles as above

            uint32_t address = sp + data.word;

            if (data.load) {
                gpr[data.rd] = bus->read_word(address, CpuAccess::Sequential);
                internal_cycle();
            } else {
                bus->write_word(address, gpr[data.rd], CpuAccess::Sequential);
            }

            // last read/write is unrelated
            next_access = CpuAccess::NonSequential;
        },
        [this](LoadAddress& data) {
            // 1S cycle in step()

            if (data.sp) {
                gpr[data.rd] = sp + data.word;
            } else {
                // PC is already current + 4, so dont need to do that
                // force bit 1 to 0
                gpr[data.rd] = (pc & ~(1 << 1)) + data.word;
            }
        },
        [this](AddOffsetStackPointer& data) {
            // 1S cycle in step()

            sp += data.word;
        },
        [this, &is_flushed](PushPopRegister& data) {
            /*
              Load
              ====
              S       -> reading instruction in step()
              N       -> unrelated read from target
              (n-1) S -> next n - 1 related reads from target
              I       -> stored in register
              N+S     -> if PC is written - taken care of by flush_pipeline()
              S       -> if PC, memory read for PC write
              Total = nS + N + I or (n+2)S + 2N + I

              Store
              =====
              N       -> calculating memory address
              N       -> if LR, memory read for PC write
              N/S     -> unrelated write at target
              (n-1) S -> next n - 1 related writes
              Total = 2N + nS or 2N + (n-1)S
            */
            static constexpr uint8_t alignment = 4;
            CpuAccess access                   = CpuAccess::NonSequential;

            if (data.load) {
                for (uint8_t i = 0; i < 8; i++) {
                    if (get_bit(data.regs, i)) {
                        gpr[i] = bus->read_word(sp, access);
                        sp += alignment;
                        access = CpuAccess::Sequential;
                    }
                }

                if (data.pclr) {
                    pc = bus->read_word(sp, access);
                    sp += alignment;
                    is_flushed = true;
                }

                // I
                internal_cycle();
            } else {
                if (data.pclr) {
                    sp -= alignment;
                    bus->write_word(sp, lr, access);
                    access = CpuAccess::Sequential;
                }

                for (int8_t i = 7; i >= 0; i--) {
                    if (get_bit(data.regs, i)) {
                        sp -= alignment;
                        bus->write_word(sp, gpr[i], access);
                        access = CpuAccess::Sequential;
                    }
                }
            }

            // last read/write is unrelated
            next_access = CpuAccess::NonSequential;
        },
        [this](MultipleLoad& data) {
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

            uint32_t rb      = gpr[data.rb];
            CpuAccess access = CpuAccess::NonSequential;

            if (data.load) {
                for (uint8_t i = 0; i < 8; i++) {
                    if (get_bit(data.regs, i)) {
                        gpr[i] = bus->read_word(rb, access);
                        rb += alignment;
                        access = CpuAccess::Sequential;
                    }
                }
                internal_cycle();
            } else {
                for (uint8_t i = 0; i < 8; i++) {
                    if (get_bit(data.regs, i)) {
                        bus->write_word(rb, gpr[i], access);
                        rb += alignment;
                        access = CpuAccess::Sequential;
                    }
                }
            }

            gpr[data.rb] = rb;

            // last read/write is unrelated
            next_access = CpuAccess::NonSequential;
        },
        [this, &is_flushed](ConditionalBranch& data) {
            /*
              S   -> reading instruction in step()
              N+S -> if condition is true, branch and refill pipeline
              Total = S or 2S + N
            */

            if (data.condition == Condition::AL)
                glogger.warn("Condition 1110 (AL) is undefined");

            if (!cpsr.condition(data.condition))
                return;

            pc += data.offset;
            is_flushed = true;
        },
        [this, &is_flushed](SoftwareInterrupt& data) {
            /*
              S   -> reading instruction in step()
              N+S -> refill pipeline
              Total = 2S + N
            */

            // next instruction is one instruction behind PC
            lr   = pc - INSTRUCTION_SIZE;
            spsr = cpsr;
            pc   = data.vector;
            cpsr.set_state(State::Arm);
            chg_mode(Mode::Supervisor);
            is_flushed = true;
        },
        [this, &is_flushed](UnconditionalBranch& data) {
            /*
              S   -> reading instruction in step()
              N+S -> branch and refill pipeline
              Total = 2S + N
            */

            pc += data.offset;
            is_flushed = true;
        },
        [this, &is_flushed](LongBranchWithLink& data) {
            /*
              S -> prefetched instruction in step()
              N -> fetch from the new address in branch
              S -> last opcode fetch at +L to refill the pipeline
              Total = 2S + N cycles
                      1S done, S+N taken care of by flush_pipeline()
            */

            // 12 bit integer
            int32_t offset = data.offset;

            if (data.low) {
                uint32_t old_pc = pc;
                offset <<= 1;

                pc         = lr + offset;
                lr         = (old_pc - INSTRUCTION_SIZE) | 1;
                is_flushed = true;
            } else {
                // 12 + 11 = 23 bit
                offset <<= 12;
                // sign extend
                offset = (offset << 9) >> 9;
                lr     = pc + offset;
            }
        },
        [](auto& data) {
            glogger.error("Unknown thumb format : {}", typeid(data).name());
        } },
      instruction.data);

    if (is_flushed)
        flush_pipeline<State::Thumb>();
    else
        advance_pc_thumb();
}
}
