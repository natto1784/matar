#pragma once
#include "cpu/alu.hh"
#include "cpu/psr.hh"
#include <cstdint>
#include <fmt/ostream.h>
#include <variant>

namespace matar {
namespace thumb {

template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

static constexpr size_t INSTRUCTION_SIZE = 2;

struct MoveShiftedRegister {
    uint8_t rd;
    uint8_t rs;
    uint8_t offset;
    ShiftType opcode;
};

struct AddSubtract {
    enum class OpCode {
        ADD = 0,
        SUB = 1
    };

    uint8_t rd;
    uint8_t rs;
    uint8_t offset;
    OpCode opcode;
    bool imm;
};

struct MovCmpAddSubImmediate {
    enum class OpCode {
        MOV = 0b00,
        CMP = 0b01,
        ADD = 0b10,
        SUB = 0b11
    };

    uint8_t offset;
    uint8_t rd;
    OpCode opcode;
};

struct AluOperations {
    enum class OpCode {
        AND = 0b0000,
        EOR = 0b0001,
        LSL = 0b0010,
        LSR = 0b0011,
        ASR = 0b0100,
        ADC = 0b0101,
        SBC = 0b0110,
        ROR = 0b0111,
        TST = 0b1000,
        NEG = 0b1001,
        CMP = 0b1010,
        CMN = 0b1011,
        ORR = 0b1100,
        MUL = 0b1101,
        BIC = 0b1110,
        MVN = 0b1111
    };

    uint8_t rd;
    uint8_t rs;
    OpCode opcode;
};

struct HiRegisterOperations {
    enum class OpCode {
        ADD = 0b00,
        CMP = 0b01,
        MOV = 0b10,
        BX  = 0b11
    };

    uint8_t rd;
    uint8_t rs;
    bool hi_2;
    bool hi_1;
    OpCode opcode;
};

struct PcRelativeLoad {
    uint8_t word;
    uint8_t rd;
};

struct LoadStoreRegisterOffset {
    uint8_t rd;
    uint8_t rb;
    uint8_t ro;
    bool byte;
    bool load;
};

struct LoadStoreSignExtendedHalfword {
    uint8_t rd;
    uint8_t rb;
    uint8_t ro;
    bool s;
    bool h;
};

struct LoadStoreImmediateOffset {
    uint8_t rd;
    uint8_t rb;
    uint8_t offset;
    bool load;
    bool byte;
};

struct LoadStoreHalfword {
    uint8_t rd;
    uint8_t rb;
    uint8_t offset;
    bool load;
};

struct SpRelativeLoad {
    uint8_t word;
    uint8_t rd;
    bool load;
};

struct LoadAddress {
    uint8_t word;
    uint8_t rd;
    bool sp;
};

struct AddOffsetStackPointer {
    uint8_t word;
    bool sign;
};

struct PushPopRegister {
    uint8_t regs;
    bool pclr;
    bool load;
};

struct MultipleLoad {
    uint8_t regs;
    uint8_t rb;
    bool load;
};

struct ConditionalBranch {
    uint8_t offset;
    Condition condition;
};

struct SoftwareInterrupt {};

struct UnconditionalBranch {
    uint16_t offset;
};

struct LongBranchWithLink {
    uint16_t offset;
    bool high;
};

using InstructionData = std::variant<MoveShiftedRegister,
                                     AddSubtract,
                                     MovCmpAddSubImmediate,
                                     AluOperations,
                                     HiRegisterOperations,
                                     PcRelativeLoad,
                                     LoadStoreRegisterOffset,
                                     LoadStoreSignExtendedHalfword,
                                     LoadStoreImmediateOffset,
                                     LoadStoreHalfword,
                                     SpRelativeLoad,
                                     LoadAddress,
                                     AddOffsetStackPointer,
                                     PushPopRegister,
                                     MultipleLoad,
                                     ConditionalBranch,
                                     SoftwareInterrupt,
                                     UnconditionalBranch,
                                     LongBranchWithLink>;

struct Instruction {
    InstructionData data;

    Instruction(uint16_t insn);

    std::string disassemble();
};

std::ostream&
operator<<(std::ostream& os, const AddSubtract::OpCode cond);

std::ostream&
operator<<(std::ostream& os, const MovCmpAddSubImmediate::OpCode cond);

std::ostream&
operator<<(std::ostream& os, const AluOperations::OpCode cond);

std::ostream&
operator<<(std::ostream& os, const HiRegisterOperations::OpCode cond);
}
}

namespace fmt {
template<>
struct formatter<matar::thumb::AddSubtract::OpCode> : ostream_formatter {};

template<>
struct formatter<matar::thumb::MovCmpAddSubImmediate::OpCode>
  : ostream_formatter {};

template<>
struct formatter<matar::thumb::AluOperations::OpCode> : ostream_formatter {};

template<>
struct formatter<matar::thumb::HiRegisterOperations::OpCode>
  : ostream_formatter {};
}
