#include "instruction.hh"
#include "util/bits.hh"

namespace matar::thumb {
Instruction::Instruction(uint16_t insn) {
    // Format 2: Add/Subtract
    if ((insn & 0xF800) == 0x1800) {
        uint8_t rd     = bit_range(insn, 0, 2);
        uint8_t rs     = bit_range(insn, 3, 5);
        uint8_t offset = bit_range(insn, 6, 8);
        AddSubtract::OpCode opcode =
          static_cast<AddSubtract::OpCode>(get_bit(insn, 9));
        bool imm = get_bit(insn, 10);

        data = AddSubtract{
            .rd = rd, .rs = rs, .offset = offset, .opcode = opcode, .imm = imm
        };

        // Format 1:  Move Shifted Register
    } else if ((insn & 0xE000) == 0x0000) {
        uint8_t rd       = bit_range(insn, 0, 2);
        uint8_t rs       = bit_range(insn, 3, 5);
        uint8_t offset   = bit_range(insn, 6, 10);
        ShiftType opcode = static_cast<ShiftType>(bit_range(insn, 11, 12));

        data = MoveShiftedRegister{
            .rd = rd, .rs = rs, .offset = offset, .opcode = opcode
        };

        // Format 3: Move/compare/add/subtract immediate
    } else if ((insn & 0xE000) == 0x2000) {
        uint8_t offset = bit_range(insn, 0, 7);
        uint8_t rd     = bit_range(insn, 8, 10);
        MovCmpAddSubImmediate::OpCode opcode =
          static_cast<MovCmpAddSubImmediate::OpCode>(bit_range(insn, 11, 12));

        data =
          MovCmpAddSubImmediate{ .offset = offset, .rd = rd, .opcode = opcode };

        // Format 4: ALU operations
    } else if ((insn & 0xFC00) == 0x4000) {
        uint8_t rd = bit_range(insn, 0, 2);
        uint8_t rs = bit_range(insn, 3, 5);
        AluOperations::OpCode opcode =
          static_cast<AluOperations::OpCode>(bit_range(insn, 6, 9));

        data = AluOperations{ .rd = rd, .rs = rs, .opcode = opcode };

        // Format 5: Hi register operations/branch exchange
    } else if ((insn & 0xFC00) == 0x4400) {
        uint8_t rd = bit_range(insn, 0, 2);
        uint8_t rs = bit_range(insn, 3, 5);
        bool hi_2  = get_bit(insn, 6);
        bool hi_1  = get_bit(insn, 7);
        HiRegisterOperations::OpCode opcode =
          static_cast<HiRegisterOperations::OpCode>(bit_range(insn, 8, 9));

        rd += (hi_1 ? LO_GPR_COUNT : 0);
        rs += (hi_2 ? LO_GPR_COUNT : 0);

        data = HiRegisterOperations{ .rd = rd, .rs = rs, .opcode = opcode };
        // Format 6: PC-relative load
    } else if ((insn & 0xF800) == 0x4800) {
        uint8_t word = bit_range(insn, 0, 7);
        uint8_t rd   = bit_range(insn, 8, 10);

        data = PcRelativeLoad{ .word = word, .rd = rd };

        // Format 7: Load/store with register offset
    } else if ((insn & 0xF200) == 0x5000) {
        uint8_t rd = bit_range(insn, 0, 2);
        uint8_t rb = bit_range(insn, 3, 5);
        uint8_t ro = bit_range(insn, 6, 8);
        bool byte  = get_bit(insn, 10);
        bool load  = get_bit(insn, 11);

        data = LoadStoreRegisterOffset{
            .rd = rd, .rb = rb, .ro = ro, .byte = byte, .load = load
        };

        // Format 8: Load/store sign-extended byte/halfword
    } else if ((insn & 0xF200) == 0x5200) {
        uint8_t rd = bit_range(insn, 0, 2);
        uint8_t rb = bit_range(insn, 3, 5);
        uint8_t ro = bit_range(insn, 6, 8);
        bool s     = get_bit(insn, 10);
        bool h     = get_bit(insn, 11);

        data = LoadStoreSignExtendedHalfword{
            .rd = rd, .rb = rb, .ro = ro, .s = s, .h = h
        };

        // Format 9: Load/store with immediate offset
    } else if ((insn & 0xF000) == 0x6000) {
        uint8_t rd     = bit_range(insn, 0, 2);
        uint8_t rb     = bit_range(insn, 3, 5);
        uint8_t offset = bit_range(insn, 6, 10);
        bool load      = get_bit(insn, 11);
        bool byte      = get_bit(insn, 12);

        data = LoadStoreImmediateOffset{
            .rd = rd, .rb = rb, .offset = offset, .load = load, .byte = byte
        };

        // Format 10: Load/store halfword
    } else if ((insn & 0xF000) == 0x8000) {
        uint8_t rd     = bit_range(insn, 0, 2);
        uint8_t rb     = bit_range(insn, 3, 5);
        uint8_t offset = bit_range(insn, 6, 10);
        bool load      = get_bit(insn, 11);

        data = LoadStoreHalfword{
            .rd = rd, .rb = rb, .offset = offset, .load = load
        };

        // Format 11: SP-relative load/store
    } else if ((insn & 0xF000) == 0x9000) {
        uint8_t word = bit_range(insn, 0, 7);
        uint8_t rd   = bit_range(insn, 8, 10);
        bool load    = get_bit(insn, 11);

        data = SpRelativeLoad{ .word = word, .rd = rd, .load = load };

        // Format 12: Load address
    } else if ((insn & 0xF000) == 0xA000) {
        uint8_t word = bit_range(insn, 0, 7);
        uint8_t rd   = bit_range(insn, 8, 10);
        bool sp      = get_bit(insn, 11);

        data = LoadAddress{ .word = word, .rd = rd, .sp = sp };

        // Format 12: Load address
    } else if ((insn & 0xF000) == 0xA000) {
        uint8_t word = bit_range(insn, 0, 7);
        uint8_t rd   = bit_range(insn, 8, 10);
        bool sp      = get_bit(insn, 11);

        data = LoadAddress{ .word = word, .rd = rd, .sp = sp };

        // Format 13: Add offset to stack pointer
    } else if ((insn & 0xFF00) == 0xB000) {
        uint8_t word = bit_range(insn, 0, 6);
        bool sign    = get_bit(insn, 7);

        data = AddOffsetStackPointer{ .word = word, .sign = sign };

        // Format 14: Push/pop registers
    } else if ((insn & 0xF600) == 0xB400) {
        uint8_t regs = bit_range(insn, 0, 7);
        bool pclr    = get_bit(insn, 8);
        bool load    = get_bit(insn, 11);

        data = PushPopRegister{ .regs = regs, .pclr = pclr, .load = load };

        // Format 15: Multiple load/store
    } else if ((insn & 0xF000) == 0xC000) {
        uint8_t regs = bit_range(insn, 0, 7);
        uint8_t rb   = bit_range(insn, 8, 10);
        bool load    = get_bit(insn, 11);

        data = MultipleLoad{ .regs = regs, .rb = rb, .load = load };

        // Format 17: Software interrupt
    } else if ((insn & 0xFF00) == 0xDF00) {
        data = SoftwareInterrupt{};

        // Format 16: Conditional branch
    } else if ((insn & 0xF000) == 0xD000) {
        uint16_t offset     = bit_range(insn, 0, 7);
        Condition condition = static_cast<Condition>(bit_range(insn, 8, 11));

        data = ConditionalBranch{ .offset = static_cast<uint16_t>(offset << 1),
                                  .condition = condition };

        // Format 18: Unconditional branch
    } else if ((insn & 0xF800) == 0xE000) {
        uint16_t offset = bit_range(insn, 0, 10);

        data =
          UnconditionalBranch{ .offset = static_cast<uint16_t>(offset << 1) };

        // Format 19: Long branch with link
    } else if ((insn & 0xF000) == 0xF000) {
        uint16_t offset = bit_range(insn, 0, 10);
        bool high       = get_bit(insn, 11);

        data = LongBranchWithLink{ .offset = static_cast<uint16_t>(offset << 1),
                                   .high   = high };
    }
}
}
