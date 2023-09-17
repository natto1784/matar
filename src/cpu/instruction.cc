#include "cpu/instruction.hh"
#include "cpu/utility.hh"
#include "util/bits.hh"
#include <iterator>

using namespace arm;

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

        offset += 2 * ARM_INSTRUCTION_SIZE;

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
        bool uns     = get_bit(insn, 22);

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

            if (!imm) {
                operand = bit_range(insn, 0, 3);
            } else {
                uint32_t immediate = bit_range(insn, 0, 7);
                uint8_t rotate     = bit_range(insn, 8, 11);

                operand = std::rotr(immediate, rotate * 2);
            }

            data = PsrTransfer{ .operand = operand,
                                .spsr    = get_bit(insn, 22),
                                .type    = (get_bit(insn, 16)
                                              ? PsrTransfer::Type::Msr
                                              : PsrTransfer::Type::Msr_flg),
                                .imm     = imm };
        } else {
            std::variant<Shift, uint32_t> operand;

            if (!imm) {
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

std::string
Instruction::disassemble() {
    // goddamn this is gore
    // TODO: make this less ugly
    return std::visit(
      overloaded{
        [this](BranchAndExchange& data) {
            return fmt::format("BX{} R{:d}", condition, data.rn);
        },
        [this](Branch& data) {
            return fmt::format(
              "B{}{} 0x{:06X}", (data.link ? "L" : ""), condition, data.offset);
        },
        [this](Multiply& data) {
            if (data.acc) {
                return fmt::format("MLA{}{} R{:d},R{:d},R{:d},R{:d}",
                                   condition,
                                   (data.set ? "S" : ""),
                                   data.rd,
                                   data.rm,
                                   data.rs,
                                   data.rn);
            } else {
                return fmt::format("MUL{}{} R{:d},R{:d},R{:d}",
                                   condition,
                                   (data.set ? "S" : ""),
                                   data.rd,
                                   data.rm,
                                   data.rs);
            }
        },
        [this](MultiplyLong& data) {
            return fmt::format("{}{}{}{} R{:d},R{:d},R{:d},R{:d}",
                               (data.uns ? 'U' : 'S'),
                               (data.acc ? "MLAL" : "MULL"),
                               condition,
                               (data.set ? "S" : ""),
                               data.rdlo,
                               data.rdhi,
                               data.rm,
                               data.rs);
        },
        [](Undefined) { return std::string("UND"); },
        [this](SingleDataSwap& data) {
            return fmt::format("SWP{}{} R{:d},R{:d},[R{:d}]",
                               condition,
                               (data.byte ? "B" : ""),
                               data.rd,
                               data.rm,
                               data.rn);
        },
        [this](SingleDataTransfer& data) {
            std::string expression;
            std::string address;

            if (const uint16_t* offset = std::get_if<uint16_t>(&data.offset)) {
                if (*offset == 0) {
                    expression = "";
                } else {
                    expression =
                      fmt::format(",{}#{:d}", (data.up ? '+' : '-'), *offset);
                }
            } else if (const Shift* shift = std::get_if<Shift>(&data.offset)) {
                // Shifts are always immediate in single data transfer
                expression = fmt::format(",{}R{:d},{} #{:d}",
                                         (data.up ? '+' : '-'),
                                         shift->rm,
                                         shift->data.type,
                                         shift->data.operand);
            }

            return fmt::format(
              "{}{}{}{} R{:d},[R{:d}{}]{}",
              (data.load ? "LDR" : "STR"),
              condition,
              (data.byte ? "B" : ""),
              (!data.pre && data.write ? "T" : ""),
              data.rd,
              data.rn,
              (data.pre ? expression : ""),
              (data.pre ? (data.write ? "!" : "") : expression));
        },
        [this](HalfwordTransfer& data) {
            std::string expression;

            if (data.imm) {
                if (data.offset == 0) {
                    expression = "";
                } else {
                    expression = fmt::format(
                      ",{}#{:d}", (data.up ? '+' : '-'), data.offset);
                }
            } else {
                expression =
                  fmt::format(",{}R{:d}", (data.up ? '+' : '-'), data.offset);
            }

            return fmt::format(
              "{}{}{}{} R{:d},[R{:d}{}]{}",
              (data.load ? "LDR" : "STR"),
              condition,
              (data.sign ? "S" : ""),
              (data.half ? 'H' : 'B'),
              data.rd,
              data.rn,
              (data.pre ? expression : ""),
              (data.pre ? (data.write ? "!" : "") : expression));
        },
        [this](BlockDataTransfer& data) {
            std::string regs;

            for (uint8_t i = 0; i < 16; i++) {
                if (get_bit(data.regs, i))
                    fmt::format_to(std::back_inserter(regs), "R{:d},", i);
            };

            regs.pop_back();

            return fmt::format("{}{}{}{} R{:d}{},{{{}}}{}",
                               (data.load ? "LDM" : "STM"),
                               condition,
                               (data.up ? 'I' : 'D'),
                               (data.pre ? 'B' : 'A'),
                               data.rn,
                               (data.write ? "!" : ""),
                               regs,
                               (data.s ? "^" : ""));
        },
        [this](PsrTransfer& data) {
            if (data.type == PsrTransfer::Type::Mrs) {
                return fmt::format("MRS{} R{:d},{}",
                                   condition,
                                   data.operand,
                                   (data.spsr ? "SPSR_all" : "CPSR_all"));
            } else {
                return fmt::format(
                  "MSR{} {}_{},{}{}",
                  condition,
                  (data.spsr ? "SPSR" : "CPSR"),
                  (data.type == PsrTransfer::Type::Msr_flg ? "flg" : "all"),
                  (data.imm ? '#' : 'R'),
                  data.operand);
            }
        },
        [this](DataProcessing& data) {
            std::string op_2;

            if (const uint32_t* operand =
                  std::get_if<uint32_t>(&data.operand)) {
                op_2 = fmt::format("#{:d}", *operand);
            } else if (const Shift* shift = std::get_if<Shift>(&data.operand)) {
                op_2 = fmt::format("R{:d},{} {}{:d}",
                                   shift->rm,
                                   shift->data.type,
                                   (shift->data.immediate ? '#' : 'R'),
                                   shift->data.operand);
            }

            switch (data.opcode) {
                case OpCode::MOV:
                case OpCode::MVN:
                    return fmt::format("{}{}{} R{:d},{}",
                                       data.opcode,
                                       condition,
                                       (data.set ? "S" : ""),
                                       data.rd,
                                       op_2);
                case OpCode::TST:
                case OpCode::TEQ:
                case OpCode::CMP:
                case OpCode::CMN:
                    return fmt::format(
                      "{}{} R{:d},{}", data.opcode, condition, data.rn, op_2);
                default:
                    return fmt::format("{}{}{} R{:d},R{:d},{}",
                                       data.opcode,
                                       condition,
                                       (data.set ? "S" : ""),
                                       data.rd,
                                       data.rn,
                                       op_2);
            }
        },
        [this](SoftwareInterrupt) { return fmt::format("SWI{}", condition); },
        [this](CoprocessorDataTransfer& data) {
            std::string expression = fmt::format(",#{:d}", data.offset);
            return fmt::format(
              "{}{}{} p{:d},c{:d},[R{:d}{}]{}",
              (data.load ? "LDC" : "STC"),
              condition,
              (data.len ? "L" : ""),
              data.cpn,
              data.crd,
              data.rn,
              (data.pre ? expression : ""),
              (data.pre ? (data.write ? "!" : "") : expression));
        },
        [this](CoprocessorDataOperation& data) {
            return fmt::format("CDP{} p{},{},c{},c{},c{},{}",
                               condition,
                               data.cpn,
                               data.cp_opc,
                               data.crd,
                               data.crn,
                               data.crm,
                               data.cp);
        },
        [this](CoprocessorRegisterTransfer& data) {
            return fmt::format("{}{} p{},{},R{},c{},c{},{}",
                               (data.load ? "MRC" : "MCR"),
                               condition,
                               data.cpn,
                               data.cp_opc,
                               data.rd,
                               data.crn,
                               data.crm,
                               data.cp);
        },
        [](auto) { return std::string("unknown instruction"); } },
      data);
}
