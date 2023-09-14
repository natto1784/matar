#include "cpu/utility.hh"
#include <cstdint>
#include <variant>

template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

class ArmInstruction {
  public:
    ArmInstruction() = delete;
    ArmInstruction(uint32_t insn);

    auto get_condition() const { return condition; }
    auto get_data() const { return data; }

    std::string disassemble();

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
        bool byte;
        bool imm;
        bool up;
        bool pre;
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
                                         CoprocessorDataTransfer,
                                         CoprocessorDataOperation,
                                         CoprocessorRegisterTransfer,
                                         Undefined,
                                         SoftwareInterrupt>;

  private:
    Condition condition;
    InstructionData data;
};
