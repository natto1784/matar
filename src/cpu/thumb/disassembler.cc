#include "cpu/thumb/instruction.hh"
#include "util/bits.hh"
#include <format>

namespace matar::thumb {
std::string
Instruction::disassemble() {
    return std::visit(
      overloaded{
        [](MoveShiftedRegister& data) {
            return std::format("{} R{:d},R{:d},#{:d}",
                               stringify(data.opcode),
                               data.rd,
                               data.rs,
                               data.offset);
        },
        [](AddSubtract& data) {
            return std::format("{} R{:d},R{:d},{}{:d}",
                               stringify(data.opcode),
                               data.rd,
                               data.rs,
                               (data.imm ? '#' : 'R'),
                               data.offset);
        },
        [](MovCmpAddSubImmediate& data) {
            return std::format(
              "{} R{:d},#{:d}", stringify(data.opcode), data.rd, data.offset);
        },
        [](AluOperations& data) {
            return std::format(
              "{} R{:d},R{:d}", stringify(data.opcode), data.rd, data.rs);
        },
        [](HiRegisterOperations& data) {
            if (data.opcode == HiRegisterOperations::OpCode::BX) {
                return std::format("{} R{:d}", stringify(data.opcode), data.rs);
            }

            return std::format(
              "{} R{:d},R{:d}", stringify(data.opcode), data.rd, data.rs);
        },

        [](PcRelativeLoad& data) {
            return std::format("LDR R{:d},[PC,#{:d}]", data.rd, data.word);
        },
        [](LoadStoreRegisterOffset& data) {
            return std::format("{}{} R{:d},[R{:d},R{:d}]",
                               (data.load ? "LDR" : "STR"),
                               (data.byte ? "B" : ""),
                               data.rd,
                               data.rb,
                               data.ro);
        },
        [](LoadStoreSignExtendedHalfword& data) {
            if (!data.s && !data.h) {
                return std::format(
                  "STRH R{:d},[R{:d},R{:d}]", data.rd, data.rb, data.ro);
            }

            return std::format("{}{} R{:d},[R{:d},R{:d}]",
                               (data.s ? "LDS" : "LDR"),
                               (data.h ? 'H' : 'B'),
                               data.rd,
                               data.rb,
                               data.ro);
        },
        [](LoadStoreImmediateOffset& data) {
            return std::format("{}{} R{:d},[R{:d},#{:d}]",
                               (data.load ? "LDR" : "STR"),
                               (data.byte ? "B" : ""),
                               data.rd,
                               data.rb,
                               data.offset);
        },
        [](LoadStoreHalfword& data) {
            return std::format("{} R{:d},[R{:d},#{:d}]",
                               (data.load ? "LDRH" : "STRH"),
                               data.rd,
                               data.rb,
                               data.offset);
        },
        [](SpRelativeLoad& data) {
            return std::format("{} R{:d},[SP,#{:d}]",
                               (data.load ? "LDR" : "STR"),
                               data.rd,
                               data.word);
        },
        [](LoadAddress& data) {
            return std::format("ADD R{:d},{},#{:d}",
                               data.rd,
                               (data.sp ? "SP" : "PC"),
                               data.word);
        },
        [](AddOffsetStackPointer& data) {
            return std::format("ADD SP,#{:d}", data.word);
        },
        [](PushPopRegister& data) {
            std::string regs;

            for (uint8_t i = 0; i < 16; i++) {
                if (get_bit(data.regs, i))
                    std::format_to(std::back_inserter(regs), "R{:d},", i);
            };

            if (data.load) {
                if (data.pclr)
                    regs += "PC";
                else
                    regs.pop_back();

                return std::format("POP {{{}}}", regs);
            } else {
                if (data.pclr)
                    regs += "LR";
                else
                    regs.pop_back();

                return std::format("PUSH {{{}}}", regs);
            }
        },
        [](MultipleLoad& data) {
            std::string regs;

            for (uint8_t i = 0; i < 16; i++) {
                if (get_bit(data.regs, i))
                    std::format_to(std::back_inserter(regs), "R{:d},", i);
            };

            regs.pop_back();

            return std::format(
              "{} R{}!,{{{}}}", (data.load ? "LDMIA" : "STMIA"), data.rb, regs);
        },
        [](SoftwareInterrupt& data) {
            return std::format("SWI {:d}", data.vector);
        },
        [](ConditionalBranch& data) {
            return std::format(
              "B{} #{:d}",
              stringify(data.condition),
              static_cast<int32_t>(data.offset + 2 * INSTRUCTION_SIZE));
        },
        [](UnconditionalBranch& data) {
            return std::format(
              "B #{:d}",
              static_cast<int32_t>(data.offset + 2 * INSTRUCTION_SIZE));
        },
        [](LongBranchWithLink& data) {
            // duh this manual be empty for H = 0
            return std::format(
              "BL{} #{:d}", (data.high ? "H" : ""), data.offset);
        },
        [](auto) { return std::string("unknown instruction"); } },
      data);
}
}
