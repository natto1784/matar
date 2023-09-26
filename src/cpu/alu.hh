#pragma once
#include <cstdint>
#include <fmt/ostream.h>

namespace matar {
enum class ShiftType {
    LSL = 0b00,
    LSR = 0b01,
    ASR = 0b10,
    ROR = 0b11
};

constexpr auto
stringify(ShiftType shift_type) {
#define CASE(type)                                                             \
    case ShiftType::type:                                                      \
        return #type;

    switch (shift_type) {
        CASE(LSL)
        CASE(LSR)
        CASE(ASR)
        CASE(ROR)
    }

#undef CASE

    return "";
}

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
}
