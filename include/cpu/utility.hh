#pragma once

#include <fmt/ostream.h>
#include <ostream>

static constexpr size_t ARM_INSTRUCTION_SIZE   = 4;
static constexpr size_t THUMB_INSTRUCTION_SIZE = 2;

enum class Mode {
    /* M[4:0] in PSR */
    User       = 0b10000,
    Fiq        = 0b10001,
    Irq        = 0b10010,
    Supervisor = 0b10011,
    Abort      = 0b10111,
    Undefined  = 0b11011,
    System     = 0b11111,
};

enum class State {
    Arm   = 0,
    Thumb = 1
};

enum class Condition {
    EQ = 0b0000,
    NE = 0b0001,
    CS = 0b0010,
    CC = 0b0011,
    MI = 0b0100,
    PL = 0b0101,
    VS = 0b0110,
    VC = 0b0111,
    HI = 0b1000,
    LS = 0b1001,
    GE = 0b1010,
    LT = 0b1011,
    GT = 0b1100,
    LE = 0b1101,
    AL = 0b1110
};

// https://fmt.dev/dev/api.html#std-ostream-support
std::ostream&
operator<<(std::ostream& os, const Condition cond);
template<>
struct fmt::formatter<Condition> : ostream_formatter {};

enum class OpCode {
    AND = 0b0000,
    EOR = 0b0001,
    SUB = 0b0010,
    RSB = 0b0011,
    ADD = 0b0100,
    ADC = 0b0101,
    SBC = 0b0110,
    RSC = 0b0111,
    TST = 0b1000,
    TEQ = 0b1001,
    CMP = 0b1010,
    CMN = 0b1011,
    ORR = 0b1100,
    MOV = 0b1101,
    BIC = 0b1110,
    MVN = 0b1111
};

enum class ShiftType {
    LSL = 0b00,
    LSR = 0b01,
    ASR = 0b10,
    ROR = 0b11
};

struct ShiftData {
    ShiftType type;
    bool immediate;
    uint8_t operand;
};

struct Shift {
    uint8_t rm;
    ShiftData data;
};

uint32_t
eval_shift(ShiftType shift_type, uint32_t value, uint8_t amount, bool& carry);

// https://fmt.dev/dev/api.html#std-ostream-support
std::ostream&
operator<<(std::ostream& os, const ShiftType cond);
template<>
struct fmt::formatter<ShiftType> : ostream_formatter {};
