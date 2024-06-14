#include "cpu/alu.hh"
#include "util/bits.hh"
#include <bit>

namespace matar {
uint32_t
eval_shift(ShiftType shift_type, uint32_t value, uint32_t amount, bool& carry) {
    uint32_t eval = 0;

    switch (shift_type) {
        case ShiftType::LSL:

            if (amount > 0 && amount <= 32)
                carry = get_bit(value, 32 - amount);
            else if (amount > 32)
                carry = 0;

            eval = value << amount;
            break;
        case ShiftType::LSR:

            if (amount > 0 && amount <= 32)
                carry = get_bit(value, amount - 1);
            else if (amount > 32)
                carry = 0;
            else
                carry = get_bit(value, 31);

            eval = value >> amount;
            break;
        case ShiftType::ASR:
            if (amount > 0 && amount <= 32)
                carry = get_bit(value, amount - 1);
            else
                carry = get_bit(value, 31);

            return static_cast<int32_t>(value) >> amount;
            break;
        case ShiftType::ROR:
            if (amount == 0) {
                eval  = (value >> 1) | (carry << 31);
                carry = get_bit(value, 0);
            } else {
                eval  = std::rotr(value, amount);
                carry = get_bit(value, (amount % 32 + 31) % 32);
            }
            break;
    }

    return eval;
}

uint32_t
sub(uint32_t a, uint32_t b, bool& carry, bool& overflow) {
    bool s1 = get_bit(a, 31);
    bool s2 = get_bit(b, 31);

    uint32_t result = a - b;

    carry    = a >= b;
    overflow = s1 != s2 && s2 == get_bit(result, 31);

    return result;
}

uint32_t
add(uint32_t a, uint32_t b, bool& carry, bool& overflow, bool c) {
    bool s1 = get_bit(a, 31);
    bool s2 = get_bit(b, 31);

    uint64_t result = a + b + c;

    carry    = get_bit(result, 32);
    overflow = s1 == s2 && s2 != get_bit(result, 31);

    return result & 0xFFFFFFFF;
}

uint32_t
sbc(uint32_t a, uint32_t b, bool& carry, bool& overflow, bool c) {
    bool s1 = get_bit(a, 31);
    bool s2 = get_bit(b, 31);

    uint64_t result = a - b - !c;

    carry    = get_bit(result, 32);
    overflow = s1 != s2 && s2 == get_bit(result, 31);

    return result & 0xFFFFFFFF;
}

uint8_t
multiplier_array_cycles(uint32_t x, bool zeroes_only) {
    // set zeroes_only to evaluate first condition that checks ones to false

    if ((!zeroes_only && (x & 0xFFFFFF00) == 0xFFFFFF00) ||
        (x & 0xFFFFFF00) == 0)
        return 1;
    if ((!zeroes_only && (x & 0xFFFF0000) == 0xFFFF0000) ||
        (x & 0xFFFF0000) == 0)
        return 2;
    if ((!zeroes_only && (x & 0xFF000000) == 0xFF000000) ||
        (x & 0xFF000000) == 0)
        return 3;
    return 4;
};

}
