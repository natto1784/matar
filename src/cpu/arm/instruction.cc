#include "instruction.hh"
#include "util/bits.hh"
#include <iterator>

namespace matar::arm {
Instruction::Instruction(uint32_t insn)
  : condition(static_cast<Condition>(bit_range(insn, 28, 31))) {
    // Branch and exhcange
    if ((insn & 0x0FFFFFF0) == 0x012FFF10) {
        uint8_t rn = insn & 0b1111;

        data = BranchAndExchange{ rn };

        // Branch
    } else if ((insn & 0x0E000000) == 0x0A000000) {
        bool link       = get_bit(insn, 24);
        uint32_t offset = bit_range(insn, 0, 23);

        // lsh 2 and sign extend the 26 bit offset to 32 bits
        offset = (static_cast<int32_t>(offset) << 8) >> 6;

        offset += 2 * INSTRUCTION_SIZE;

        data = Branch{ .link = link, .offset = offset };

        // Multiply
    } else if ((insn & 0x0FC000F0) == 0x00000090) {
        uint8_t rm = bit_range(insn, 0, 3);
        uint8_t rs = bit_range(insn, 8, 11);
        uint8_t rn = bit_range(insn, 12, 15);
        uint8_t rd = bit_range(insn, 16, 19);
        bool set   = get_bit(insn, 20);
        bool acc   = get_bit(insn, 21);

        data = Multiply{
            .rm = rm, .rs = rs, .rn = rn, .rd = rd, .set = set, .acc = acc
        };

        // Multiply long
    } else if ((insn & 0x0F8000F0) == 0x00800090) {
        uint8_t rm   = bit_range(insn, 0, 3);
        uint8_t rs   = bit_range(insn, 8, 11);
        uint8_t rdlo = bit_range(insn, 12, 15);
        uint8_t rdhi = bit_range(insn, 16, 19);
        bool set     = get_bit(insn, 20);
        bool acc     = get_bit(insn, 21);
        bool uns     = !get_bit(insn, 22);

        data = MultiplyLong{ .rm   = rm,
                             .rs   = rs,
                             .rdlo = rdlo,
                             .rdhi = rdhi,
                             .set  = set,
                             .acc  = acc,
                             .uns  = uns };

        // Undefined
    } else if ((insn & 0x0E000010) == 0x06000010) {
        data = Undefined{};

        // Single data swap
    } else if ((insn & 0x0FB00FF0) == 0x01000090) {
        uint8_t rm = bit_range(insn, 0, 3);
        uint8_t rd = bit_range(insn, 12, 15);
        uint8_t rn = bit_range(insn, 16, 19);
        bool byte  = get_bit(insn, 22);

        data = SingleDataSwap{ .rm = rm, .rd = rd, .rn = rn, .byte = byte };

        // Single data transfer
    } else if ((insn & 0x0C000000) == 0x04000000) {
        std::variant<uint16_t, Shift> offset;
        uint8_t rd = bit_range(insn, 12, 15);
        uint8_t rn = bit_range(insn, 16, 19);
        bool load  = get_bit(insn, 20);
        bool write = get_bit(insn, 21);
        bool byte  = get_bit(insn, 22);
        bool up    = get_bit(insn, 23);
        bool pre   = get_bit(insn, 24);
        bool imm   = get_bit(insn, 25);

        if (imm) {
            // register specified shift amounts not available in single data
            // transfer (see Undefined)
            uint8_t rm = bit_range(insn, 0, 3);
            ShiftType shift_type =
              static_cast<ShiftType>(bit_range(insn, 5, 6));
            uint8_t operand = bit_range(insn, 7, 11);

            offset = Shift{ .rm   = rm,
                            .data = ShiftData{ .type      = shift_type,
                                               .immediate = true,
                                               .operand   = operand } };
        } else {
            offset = static_cast<uint16_t>(bit_range(insn, 0, 11));
        }

        data = SingleDataTransfer{ .offset = offset,
                                   .rd     = rd,
                                   .rn     = rn,
                                   .load   = load,
                                   .write  = write,
                                   .byte   = byte,
                                   .up     = up,
                                   .pre    = pre };

        // Halfword transfer
    } else if ((insn & 0x0E000090) == 0x00000090) {
        uint8_t offset = bit_range(insn, 0, 3);
        bool half      = get_bit(insn, 5);
        bool sign      = get_bit(insn, 6);
        uint8_t rd     = bit_range(insn, 12, 15);
        uint8_t rn     = bit_range(insn, 16, 19);
        bool load      = get_bit(insn, 20);
        bool write     = get_bit(insn, 21);
        bool imm       = get_bit(insn, 22);
        bool up        = get_bit(insn, 23);
        bool pre       = get_bit(insn, 24);

        offset |= (imm ? bit_range(insn, 8, 11) << 2 : 0);

        data = HalfwordTransfer{ .offset = offset,
                                 .half   = half,
                                 .sign   = sign,
                                 .rd     = rd,
                                 .rn     = rn,
                                 .load   = load,
                                 .write  = write,
                                 .imm    = imm,
                                 .up     = up,
                                 .pre    = pre };

        // Block data transfer
    } else if ((insn & 0x0E000000) == 0x08000000) {
        uint16_t regs = bit_range(insn, 0, 15);
        uint8_t rn    = bit_range(insn, 16, 19);
        bool load     = get_bit(insn, 20);
        bool write    = get_bit(insn, 21);
        bool s        = get_bit(insn, 22);
        bool up       = get_bit(insn, 23);
        bool pre      = get_bit(insn, 24);

        data = BlockDataTransfer{ .regs  = regs,
                                  .rn    = rn,
                                  .load  = load,
                                  .write = write,
                                  .s     = s,
                                  .up    = up,
                                  .pre   = pre };

        // Data Processing
    } else if ((insn & 0x0C000000) == 0x00000000) {
        using OpCode = DataProcessing::OpCode;

        uint8_t rd    = bit_range(insn, 12, 15);
        uint8_t rn    = bit_range(insn, 16, 19);
        bool set      = get_bit(insn, 20);
        OpCode opcode = static_cast<OpCode>(bit_range(insn, 21, 24));
        bool imm      = get_bit(insn, 25);

        if ((opcode == OpCode::TST || opcode == OpCode::CMP) && !set) {
            data = PsrTransfer{ .operand = rd,
                                .spsr    = get_bit(insn, 22),
                                .type    = PsrTransfer::Type::Mrs,
                                .imm     = 0 };
        } else if ((opcode == OpCode::TEQ || opcode == OpCode::CMN) && !set) {
            uint32_t operand = 0;

            if (imm) {
                uint32_t immediate = bit_range(insn, 0, 7);
                uint8_t rotate     = bit_range(insn, 8, 11);

                operand = std::rotr(immediate, rotate * 2);
            } else {
                operand = bit_range(insn, 0, 3);
            }

            data = PsrTransfer{ .operand = operand,
                                .spsr    = get_bit(insn, 22),
                                .type    = (get_bit(insn, 16)
                                              ? PsrTransfer::Type::Msr
                                              : PsrTransfer::Type::Msr_flg),
                                .imm     = imm };
        } else {
            std::variant<Shift, uint32_t> operand;

            if (imm) {
                uint32_t immediate = bit_range(insn, 0, 7);
                uint8_t rotate     = bit_range(insn, 8, 11);

                operand = std::rotr(immediate, rotate * 2);
            } else {
                uint8_t rm = bit_range(insn, 0, 3);
                bool reg   = get_bit(insn, 4);
                ShiftType shift_type =
                  static_cast<ShiftType>(bit_range(insn, 5, 6));
                uint8_t sh_operand = bit_range(insn, (reg ? 8 : 7), 11);

                operand = Shift{ .rm   = rm,
                                 .data = ShiftData{ .type      = shift_type,
                                                    .immediate = !reg,
                                                    .operand   = sh_operand } };
            }

            data = DataProcessing{ .operand = operand,
                                   .rd      = rd,
                                   .rn      = rn,
                                   .set     = set,
                                   .opcode  = opcode };
        }

        // Software interrupt
    } else if ((insn & 0x0F000000) == 0x0F000000) {

        data = SoftwareInterrupt{};

        // Coprocessor data transfer
    } else if ((insn & 0x0E000000) == 0x0C000000) {
        uint8_t offset = bit_range(insn, 0, 7);
        uint8_t cpn    = bit_range(insn, 8, 11);
        uint8_t crd    = bit_range(insn, 12, 15);
        uint8_t rn     = bit_range(insn, 16, 19);
        bool load      = get_bit(insn, 20);
        bool write     = get_bit(insn, 21);
        bool len       = get_bit(insn, 22);
        bool up        = get_bit(insn, 23);
        bool pre       = get_bit(insn, 24);

        data = CoprocessorDataTransfer{ .offset = offset,
                                        .cpn    = cpn,
                                        .crd    = crd,
                                        .rn     = rn,
                                        .load   = load,
                                        .write  = write,
                                        .len    = len,
                                        .up     = up,
                                        .pre    = pre };

        // Coprocessor data operation
    } else if ((insn & 0x0F000010) == 0x0E000000) {
        uint8_t crm    = bit_range(insn, 0, 3);
        uint8_t cp     = bit_range(insn, 5, 7);
        uint8_t cpn    = bit_range(insn, 8, 11);
        uint8_t crd    = bit_range(insn, 12, 15);
        uint8_t crn    = bit_range(insn, 16, 19);
        uint8_t cp_opc = bit_range(insn, 20, 23);

        data = CoprocessorDataOperation{ .crm    = crm,
                                         .cp     = cp,
                                         .cpn    = cpn,
                                         .crd    = crd,
                                         .crn    = crn,
                                         .cp_opc = cp_opc };

        // Coprocessor register transfer
    } else if ((insn & 0x0F000010) == 0x0E000010) {
        uint8_t crm    = bit_range(insn, 0, 3);
        uint8_t cp     = bit_range(insn, 5, 7);
        uint8_t cpn    = bit_range(insn, 8, 11);
        uint8_t rd     = bit_range(insn, 12, 15);
        uint8_t crn    = bit_range(insn, 16, 19);
        bool load      = get_bit(insn, 20);
        uint8_t cp_opc = bit_range(insn, 21, 23);

        data = CoprocessorRegisterTransfer{ .crm    = crm,
                                            .cp     = cp,
                                            .cpn    = cpn,
                                            .rd     = rd,
                                            .crn    = crn,
                                            .load   = load,
                                            .cp_opc = cp_opc };
    } else {
        data = Undefined{};
    }
}
}
