#include "instruction.hh"
#include "util/bits.hh"

ArmInstruction::ArmInstruction(uint32_t insn)
  : condition(static_cast<Condition>(get_bit_range(insn, 28, 31))) {
    // Branch and exhcange
    if ((insn & 0x0FFFFFF0) == 0x012FFF10) {
        uint8_t rn = insn & 0b1111;

        data = BranchAndExchange{ rn };

        // Branch
    } else if ((insn & 0x0E000000) == 0x0A000000) {
        bool link       = get_nth_bit(insn, 24);
        uint32_t offset = get_bit_range(insn, 0, 23);

        data = Branch{ .link = link, .offset = offset };

        // Multiply
    } else if ((insn & 0x0FC000F0) == 0x00000090) {
        uint8_t rm = get_bit_range(insn, 0, 3);
        uint8_t rs = get_bit_range(insn, 8, 11);
        uint8_t rn = get_bit_range(insn, 12, 15);
        uint8_t rd = get_bit_range(insn, 16, 19);
        bool set   = get_nth_bit(insn, 20);
        bool acc   = get_nth_bit(insn, 21);

        data = Multiply{
            .rm = rm, .rs = rs, .rn = rn, .rd = rd, .set = set, .acc = acc
        };

        // Multiply long
    } else if ((insn & 0x0F8000F0) == 0x00800090) {
        uint8_t rm   = get_bit_range(insn, 0, 3);
        uint8_t rs   = get_bit_range(insn, 8, 11);
        uint8_t rdlo = get_bit_range(insn, 12, 15);
        uint8_t rdhi = get_bit_range(insn, 16, 19);
        bool set     = get_nth_bit(insn, 20);
        bool acc     = get_nth_bit(insn, 21);
        bool uns     = get_nth_bit(insn, 22);

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
        uint8_t rm = get_bit_range(insn, 0, 3);
        uint8_t rd = get_bit_range(insn, 12, 15);
        uint8_t rn = get_bit_range(insn, 16, 19);
        bool byte  = get_nth_bit(insn, 22);

        data = SingleDataSwap{ .rm = rm, .rd = rd, .rn = rn, .byte = byte };

        // Single data transfer
    } else if ((insn & 0x0C000000) == 0x04000000) {

        std::variant<uint16_t, Shift> offset;
        uint8_t rd = get_bit_range(insn, 12, 15);
        uint8_t rn = get_bit_range(insn, 16, 19);
        bool load  = get_nth_bit(insn, 20);
        bool write = get_nth_bit(insn, 21);
        bool byte  = get_nth_bit(insn, 22);
        bool up    = get_nth_bit(insn, 23);
        bool pre   = get_nth_bit(insn, 24);
        bool imm   = get_nth_bit(insn, 25);

        if (imm) {
            uint8_t rm = get_bit_range(insn, 0, 3);
            bool reg   = get_nth_bit(insn, 4);
            ShiftType shift_type =
              static_cast<ShiftType>(get_bit_range(insn, 5, 6));
            uint8_t operand = get_bit_range(insn, (reg ? 8 : 7), 11);

            offset = Shift{ .rm   = rm,
                            .data = ShiftData{ .type      = shift_type,
                                               .immediate = !reg,
                                               .operand   = operand } };
        } else {
            offset = static_cast<uint16_t>(get_bit_range(insn, 0, 11));
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
        uint8_t offset = get_bit_range(insn, 0, 3);
        bool half      = get_nth_bit(insn, 5);
        bool sign      = get_nth_bit(insn, 6);
        uint8_t rd     = get_bit_range(insn, 12, 15);
        uint8_t rn     = get_bit_range(insn, 16, 19);
        bool load      = get_nth_bit(insn, 20);
        bool write     = get_nth_bit(insn, 21);
        bool imm       = get_nth_bit(insn, 22);
        bool up        = get_nth_bit(insn, 23);
        bool pre       = get_nth_bit(insn, 24);

        offset |= (imm ? get_bit_range(insn, 8, 11) << 2 : 0);

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
        /*static constexpr array<stringv, 2> syn = { "STM", "LDM" };

        uint16_t regs = get_bit_range(insn, 0, 15);
        uint8_t rn    = get_bit_range(insn, 16, 19);
        bool load     = get_nth_bit(insn, 20);
        bool write    = get_nth_bit(insn, 21);
        bool s        = get_nth_bit(insn, 22);
        bool up       = get_nth_bit(insn, 23);
        bool pre      = get_nth_bit(insn, 24);

        // disassembly
        {
            uint8_t lpu = load << 2 | pre << 1 | up;
            std::string addr_mode;

            switch(lpu) {
            }
            }*/

        data = Undefined{};

        // Software Interrupt
        // What to do here?
    } else if ((insn & 0x0F000000) == 0x0F000000) {

        data = SoftwareInterrupt{};

        // Coprocessor data transfer
    } else if ((insn & 0x0E000000) == 0x0C000000) {
        uint8_t offset = get_bit_range(insn, 0, 7);
        uint8_t cpn    = get_bit_range(insn, 8, 11);
        uint8_t crd    = get_bit_range(insn, 12, 15);
        uint8_t rn     = get_bit_range(insn, 16, 19);
        bool load      = get_nth_bit(insn, 20);
        bool write     = get_nth_bit(insn, 21);
        bool len       = get_nth_bit(insn, 22);
        bool up        = get_nth_bit(insn, 23);
        bool pre       = get_nth_bit(insn, 24);

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
        uint8_t crm    = get_bit_range(insn, 0, 4);
        uint8_t cp     = get_bit_range(insn, 5, 7);
        uint8_t cpn    = get_bit_range(insn, 8, 11);
        uint8_t crd    = get_bit_range(insn, 12, 15);
        uint8_t crn    = get_bit_range(insn, 16, 19);
        uint8_t cp_opc = get_bit_range(insn, 20, 23);

        data = CoprocessorDataOperation{ .crm    = crm,
                                         .cp     = cp,
                                         .cpn    = cpn,
                                         .crd    = crd,
                                         .crn    = crn,
                                         .cp_opc = cp_opc };

        // Coprocessor register transfer
    } else if ((insn & 0x0F000010) == 0x0E000010) {
        uint8_t crm    = get_bit_range(insn, 0, 4);
        uint8_t cp     = get_bit_range(insn, 5, 7);
        uint8_t cpn    = get_bit_range(insn, 8, 11);
        uint8_t rd     = get_bit_range(insn, 12, 15);
        uint8_t crn    = get_bit_range(insn, 16, 19);
        bool load      = get_nth_bit(insn, 20);
        uint8_t cp_opc = get_bit_range(insn, 21, 23);

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
ArmInstruction::disassemble() {
    static const std::string undefined = "UNDEFINED";
    // goddamn this is gore
    // TODO: make this less ugly
    return std::visit(
      overloaded{
        [this](BranchAndExchange& data) {
            return fmt::format("BX{} R{:d}", condition, data.rn);
        },
        [this](Branch& data) {
            return fmt::format(
              "B{}{} {:#08X}", (data.link ? "L" : ""), condition, data.offset);
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
        [](Undefined) { return undefined; },
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
                    expression = fmt::format(",#{:d}", *offset);
                }
            } else if (const Shift* shift = std::get_if<Shift>(&data.offset)) {
                expression = fmt::format(",{}R{:d},{} {}{:d}",
                                         (data.up ? '+' : '-'),
                                         shift->rm,
                                         shift->data.type,
                                         (shift->data.immediate ? '#' : 'R'),
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
                    expression = fmt::format(",#{:d}", data.offset);
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
            return fmt::format("{}{} p{},{},c{},c{},c{},{}",
                               (data.load ? "MRC" : "MCR"),
                               condition,
                               data.cpn,
                               data.cp_opc,
                               data.rd,
                               data.crn,
                               data.crm,
                               data.cp);
        },
        [](auto) { return undefined; } },
      data);
}
