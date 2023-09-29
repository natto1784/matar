#include "instruction.hh"
#include "util/bits.hh"

namespace matar::thumb {
std::string
Instruction::disassemble(uint32_t pc) {
    return std::visit(
      overloaded{
        [](MoveShiftedRegister& data) {
            return fmt::format("{} R{:d},R{:d},#{:d}",
                               stringify(data.opcode),
                               data.rd,
                               data.rs,
                               data.offset);
        },
        [](AddSubtract& data) {
            return fmt::format("{} R{:d},R{:d},{}{:d}",
                               stringify(data.opcode),
                               data.rd,
                               data.rs,
                               (data.imm ? '#' : 'R'),
                               data.offset);
        },
        [](MovCmpAddSubImmediate& data) {
            return fmt::format(
              "{} R{:d},#{:d}", stringify(data.opcode), data.rd, data.offset);
        },
        [](AluOperations& data) {
            return fmt::format(
              "{} R{:d},R{:d}", stringify(data.opcode), data.rd, data.rs);
        },
        [](HiRegisterOperations& data) {
            if (data.opcode == HiRegisterOperations::OpCode::BX) {
                return fmt::format("{} R{:d}", stringify(data.opcode), data.rs);
            }

            return fmt::format(
              "{} R{:d},R{:d}", stringify(data.opcode), data.rd, data.rs);
        },

        [](PcRelativeLoad& data) {
            return fmt::format("LDR R{:d},[PC,#{:d}]", data.rd, data.word);
        },
        [](LoadStoreRegisterOffset& data) {
            return fmt::format("{}{} R{:d},[R{:d},R{:d}]",
                               (data.load ? "LDR" : "STR"),
                               (data.byte ? "B" : ""),
                               data.rd,
                               data.rb,
                               data.ro);
        },
        [](LoadStoreSignExtendedHalfword& data) {
            if (!data.s && !data.h) {
                return fmt::format(
                  "STRH R{:d},[R{:d},R{:d}]", data.rd, data.rb, data.ro);
            }

            return fmt::format("{}{} R{:d},[R{:d},R{:d}]",
                               (data.s ? "LDS" : "LDR"),
                               (data.h ? 'H' : 'B'),
                               data.rd,
                               data.rb,
                               data.ro);
        },
        [](LoadStoreImmediateOffset& data) {
            return fmt::format("{}{} R{:d},[R{:d},#{:d}]",
                               (data.load ? "LDR" : "STR"),
                               (data.byte ? "B" : ""),
                               data.rd,
                               data.rb,
                               data.offset);
        },
        [](LoadStoreHalfword& data) {
            return fmt::format("{} R{:d},[R{:d},#{:d}]",
                               (data.load ? "LDRH" : "STRH"),
                               data.rd,
                               data.rb,
                               data.offset);
        },
        [](SpRelativeLoad& data) {
            return fmt::format("{} R{:d},[SP,#{:d}]",
                               (data.load ? "LDR" : "STR"),
                               data.rd,
                               data.word);
        },
        [](LoadAddress& data) {
            return fmt::format("ADD R{:d},{},#{:d}",
                               data.rd,
                               (data.sp ? "SP" : "PC"),
                               data.word);
        },
        [](AddOffsetStackPointer& data) {
            return fmt::format("ADD SP,#{:d}", data.word);
        },
        [](PushPopRegister& data) {
            std::string regs;

            for (uint8_t i = 0; i < 16; i++) {
                if (get_bit(data.regs, i))
                    fmt::format_to(std::back_inserter(regs), "R{:d},", i);
            };

            if (data.load) {
                if (data.pclr)
                    regs += "PC";
                else
                    regs.pop_back();

                return fmt::format("POP {{{}}}", regs);
            } else {
                if (data.pclr)
                    regs += "LR";
                else
                    regs.pop_back();

                return fmt::format("PUSH {{{}}}", regs);
            }
        },
        [](MultipleLoad& data) {
            std::string regs;

            for (uint8_t i = 0; i < 16; i++) {
                if (get_bit(data.regs, i))
                    fmt::format_to(std::back_inserter(regs), "R{:d},", i);
            };

            regs.pop_back();

            return fmt::format(
              "{} R{}!,{{{}}}", (data.load ? "LDMIA" : "STMIA"), data.rb, regs);
        },
        [](SoftwareInterrupt& data) {
            return fmt::format("SWI {:d}", data.vector);
        },
        [pc](ConditionalBranch& data) {
            return fmt::format(
              "B{} #{:d}",
              stringify(data.condition),
              static_cast<int32_t>(data.offset + pc + 2 * INSTRUCTION_SIZE));
        },
        [pc](UnconditionalBranch& data) {
            return fmt::format(
              "B #{:d}",
              static_cast<int32_t>(data.offset + pc + 2 * INSTRUCTION_SIZE));
        },
        [](LongBranchWithLink& data) {
            // duh this manual be empty for H = 0
            return fmt::format(
              "BL{} #{:d}", (data.high ? "H" : ""), data.offset);
        },
        [](auto) { return std::string("unknown instruction"); } },
      data);
}
}
