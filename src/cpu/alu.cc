#include "cpu/alu.hh"
#include "util/bits.hh"
#include "util/log.hh"
#include <bit>

namespace matar {
uint32_t
eval_shift(ShiftType shift_type,
           bool immediate [[maybe_unused]],
           uint32_t value,
           uint32_t amount,
           bool& carry) {
    switch (shift_type) {
        case ShiftType::LSL:
            if (amount == 0) {
                return value;
            }

            if (amount < 32) {
                carry = get_bit(value, 32 - amount);
                return value << amount;
            }

            if (amount == 32) {
                carry = get_bit(value, 0);
            } else {
                carry = 0;
            }

            return 0;
        case ShiftType::LSR:
            if (amount == 0) {
                if (!immediate) {
                    return value;
                }

                // LSR #0 encodes LSR #32
                carry = get_bit(value, 31);
                return 0;
            }

            if (amount < 32) {
                carry = get_bit(value, amount - 1);
                return value >> amount;
            }

            if (amount == 32) {
                carry = get_bit(value, 31);
            } else {
                carry = 0;
            }

            return 0;
        case ShiftType::ASR:
            if (amount == 0) {
                if (!immediate)
                    return value;

                // ASR #0 encodes ASR #32
                carry = get_bit(value, 31);
                return get_bit(value, 31) ? 0xFFFFFFFF : 0;
            }

            if (amount < 32) {
                carry = get_bit(value, amount - 1);
                return static_cast<uint32_t>(static_cast<int32_t>(value) >>
                                             amount);
            }

            carry = get_bit(value, 31);

            if (carry) {
                return 0xFFFFFFFF;
            }

            return 0;
        case ShiftType::ROR:
            if (amount == 0) {
                if (!immediate) {
                    return value;
                }

                uint32_t old_c = carry;
                carry          = get_bit(value, 0);
                return (value >> 1) | (old_c << 31);
            }

            if (amount % 32 == 0) {
                carry = get_bit(value, 31);
                return value;
            }

            carry = get_bit(value, (amount & 31) - 1);
            return std::rotr(value, static_cast<int>(amount));
    }
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

    uint64_t result = static_cast<uint64_t>(a) + b + c;

    carry    = get_bit(result, 32);
    overflow = s1 == s2 && s2 != get_bit(result, 31);

    return result & 0xFFFFFFFF;
}

uint32_t
sbc(uint32_t a, uint32_t b, bool& carry, bool& overflow, bool c) {
    bool s1 = get_bit(a, 31);
    bool s2 = get_bit(b, 31);

    uint64_t result = static_cast<uint64_t>(a) - b - !c;

    carry    = !get_bit(result, 32);
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
