#pragma once
#include <cstdint>

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
eval_shift(ShiftType shift_type, uint32_t value, uint32_t amount, bool& carry);

uint32_t
sub(uint32_t a, uint32_t b, bool& carry, bool& overflow);

uint32_t
add(uint32_t a, uint32_t b, bool& carry, bool& overflow, bool c = 0);

uint32_t
sbc(uint32_t a, uint32_t b, bool& carry, bool& overflow, bool c);

uint8_t
multiplier_array_cycles(uint32_t x, bool zeroes_only = false);
}
