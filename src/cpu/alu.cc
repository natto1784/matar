#include "alu.hh"
#include "util/bits.hh"

namespace matar {
uint32_t
eval_shift(ShiftType shift_type, uint32_t value, uint8_t amount, bool& carry) {
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
}
