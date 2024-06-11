#pragma once

#include "cpu/alu.hh"
#include "cpu/psr.hh"
#include <cstdint>
#include <string>
#include <variant>

namespace matar {
class Cpu;

namespace thumb {

// https://en.cppreference.com/w/cpp/utility/variant/visit
template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

static constexpr size_t INSTRUCTION_SIZE = 2;
static constexpr uint8_t LO_GPR_COUNT    = 8;

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

constexpr auto
stringify(AddSubtract::OpCode opcode) {
#define CASE(opcode)                                                           \
    case AddSubtract::OpCode::opcode:                                          \
        return #opcode;

    switch (opcode) {
        CASE(ADD)
        CASE(SUB)
    }

#undef CASE
    return "";
}

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

constexpr auto
stringify(MovCmpAddSubImmediate::OpCode opcode) {
#define CASE(opcode)                                                           \
    case MovCmpAddSubImmediate::OpCode::opcode:                                \
        return #opcode;

    switch (opcode) {
        CASE(MOV)
        CASE(CMP)
        CASE(ADD)
        CASE(SUB)
    }

#undef CASE
    return "";
}

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

constexpr auto
stringify(AluOperations::OpCode opcode) {

#define CASE(opcode)                                                           \
    case AluOperations::OpCode::opcode:                                        \
        return #opcode;

    switch (opcode) {
        CASE(AND)
        CASE(EOR)
        CASE(LSL)
        CASE(LSR)
        CASE(ASR)
        CASE(ADC)
        CASE(SBC)
        CASE(ROR)
        CASE(TST)
        CASE(NEG)
        CASE(CMP)
        CASE(CMN)
        CASE(ORR)
        CASE(MUL)
        CASE(BIC)
        CASE(MVN)
    }

#undef CASE
    return "";
}

struct HiRegisterOperations {
    enum class OpCode {
        ADD = 0b00,
        CMP = 0b01,
        MOV = 0b10,
        BX  = 0b11
    };

    uint8_t rd;
    uint8_t rs;
    OpCode opcode;
};

constexpr auto
stringify(HiRegisterOperations::OpCode opcode) {
#define CASE(opcode)                                                           \
    case HiRegisterOperations::OpCode::opcode:                                 \
        return #opcode;

    switch (opcode) {
        CASE(ADD)
        CASE(CMP)
        CASE(MOV)
        CASE(BX)
    }

#undef CASE
    return "";
}

struct PcRelativeLoad {
    uint16_t word;
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
    uint16_t word;
    uint8_t rd;
    bool load;
};

struct LoadAddress {
    uint16_t word;
    uint8_t rd;
    bool sp;
};

struct AddOffsetStackPointer {
    int16_t word;
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
    int32_t offset;
    Condition condition;
};

struct SoftwareInterrupt {
    uint8_t vector;
};

struct UnconditionalBranch {
    int32_t offset;
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
    Instruction(uint16_t insn);
    Instruction(InstructionData data)
      : data(data) {}

    void exec(Cpu& cpu);

#ifdef DISASSEMBLER
    std::string disassemble(uint32_t pc = 0);
#endif

    InstructionData data;
};
}
}
