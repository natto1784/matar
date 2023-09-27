#include "instruction.hh"
#include "util/bits.hh"

namespace matar::arm {
std::string
Instruction::disassemble() {
    auto condition = stringify(this->condition);

    return std::visit(
      overloaded{
        [condition](BranchAndExchange& data) {
            return fmt::format("BX{} R{:d}", condition, data.rn);
        },
        [condition](Branch& data) {
            return fmt::format(
              "B{}{} 0x{:06X}", (data.link ? "L" : ""), condition, data.offset);
        },
        [condition](Multiply& data) {
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
        [condition](MultiplyLong& data) {
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
        [condition](SingleDataSwap& data) {
            return fmt::format("SWP{}{} R{:d},R{:d},[R{:d}]",
                               condition,
                               (data.byte ? "B" : ""),
                               data.rd,
                               data.rm,
                               data.rn);
        },
        [condition](SingleDataTransfer& data) {
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
                                         stringify(shift->data.type),
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
        [condition](HalfwordTransfer& data) {
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
        [condition](BlockDataTransfer& data) {
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
        [condition](PsrTransfer& data) {
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
        [condition](DataProcessing& data) {
            using OpCode = DataProcessing::OpCode;

            std::string op_2;

            if (const uint32_t* operand =
                  std::get_if<uint32_t>(&data.operand)) {
                op_2 = fmt::format("#{:d}", *operand);
            } else if (const Shift* shift = std::get_if<Shift>(&data.operand)) {
                op_2 = fmt::format("R{:d},{} {}{:d}",
                                   shift->rm,
                                   stringify(shift->data.type),
                                   (shift->data.immediate ? '#' : 'R'),
                                   shift->data.operand);
            }

            switch (data.opcode) {
                case OpCode::MOV:
                case OpCode::MVN:
                    return fmt::format("{}{}{} R{:d},{}",
                                       stringify(data.opcode),
                                       condition,
                                       (data.set ? "S" : ""),
                                       data.rd,
                                       op_2);
                case OpCode::TST:
                case OpCode::TEQ:
                case OpCode::CMP:
                case OpCode::CMN:
                    return fmt::format("{}{} R{:d},{}",
                                       stringify(data.opcode),
                                       condition,
                                       data.rn,
                                       op_2);
                default:
                    return fmt::format("{}{}{} R{:d},R{:d},{}",
                                       stringify(data.opcode),
                                       condition,
                                       (data.set ? "S" : ""),
                                       data.rd,
                                       data.rn,
                                       op_2);
            }
        },
        [condition](SoftwareInterrupt) {
            return fmt::format("SWI{}", condition);
        },
        [condition](CoprocessorDataTransfer& data) {
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
        [condition](CoprocessorDataOperation& data) {
            return fmt::format("CDP{} p{},{},c{},c{},c{},{}",
                               condition,
                               data.cpn,
                               data.cp_opc,
                               data.crd,
                               data.crn,
                               data.crm,
                               data.cp);
        },
        [condition](CoprocessorRegisterTransfer& data) {
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
}
