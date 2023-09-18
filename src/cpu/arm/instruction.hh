#include "cpu/utility.hh"
#include <cstdint>
#include <variant>

template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

namespace arm {
static constexpr size_t INSTRUCTION_SIZE = 4;

struct BranchAndExchange {
    uint8_t rn;
};

struct Branch {
    bool link;
    uint32_t offset;
};

struct Multiply {
    uint8_t rm;
    uint8_t rs;
    uint8_t rn;
    uint8_t rd;
    bool set;
    bool acc;
};

struct MultiplyLong {
    uint8_t rm;
    uint8_t rs;
    uint8_t rdlo;
    uint8_t rdhi;
    bool set;
    bool acc;
    bool uns;
};

struct SingleDataSwap {
    uint8_t rm;
    uint8_t rd;
    uint8_t rn;
    bool byte;
};

struct SingleDataTransfer {
    std::variant<uint16_t, Shift> offset;
    uint8_t rd;
    uint8_t rn;
    bool load;
    bool write;
    bool byte;
    bool up;
    bool pre;
};

struct HalfwordTransfer {
    uint8_t offset;
    bool half;
    bool sign;
    uint8_t rd;
    uint8_t rn;
    bool load;
    bool write;
    bool imm;
    bool up;
    bool pre;
};

struct BlockDataTransfer {
    uint16_t regs;
    uint8_t rn;
    bool load;
    bool write;
    bool s;
    bool up;
    bool pre;
};

struct DataProcessing {
    std::variant<Shift, uint32_t> operand;
    uint8_t rd;
    uint8_t rn;
    bool set;
    OpCode opcode;
};

struct PsrTransfer {
    enum class Type {
        Mrs,
        Msr,
        Msr_flg
    };

    uint32_t operand;
    bool spsr;
    Type type;
    // ignored outside MSR_flg
    bool imm;
};

struct CoprocessorDataTransfer {
    uint8_t offset;
    uint8_t cpn;
    uint8_t crd;
    uint8_t rn;
    bool load;
    bool write;
    bool len;
    bool up;
    bool pre;
};

struct CoprocessorDataOperation {
    uint8_t crm;
    uint8_t cp;
    uint8_t cpn;
    uint8_t crd;
    uint8_t crn;
    uint8_t cp_opc;
};

struct CoprocessorRegisterTransfer {
    uint8_t crm;
    uint8_t cp;
    uint8_t cpn;
    uint8_t rd;
    uint8_t crn;
    bool load;
    uint8_t cp_opc;
};

struct Undefined {};
struct SoftwareInterrupt {};

using InstructionData = std::variant<BranchAndExchange,
                                     Branch,
                                     Multiply,
                                     MultiplyLong,
                                     SingleDataSwap,
                                     SingleDataTransfer,
                                     HalfwordTransfer,
                                     BlockDataTransfer,
                                     DataProcessing,
                                     PsrTransfer,
                                     CoprocessorDataTransfer,
                                     CoprocessorDataOperation,
                                     CoprocessorRegisterTransfer,
                                     Undefined,
                                     SoftwareInterrupt>;

struct Instruction {
    Condition condition;
    InstructionData data;

    Instruction(uint32_t insn);
    Instruction(Condition condition, InstructionData data) noexcept
      : condition(condition)
      , data(data){};

    std::string disassemble();
};
}
