#include "cpu/utility.hh"
#include "util/bits.hh"
#include <bit>

std::ostream&
operator<<(std::ostream& os, const Condition cond) {

#define CASE(cond)                                                             \
    case Condition::cond:                                                      \
        os << #cond;                                                           \
        break;

    switch (cond) {
        CASE(EQ)
        CASE(NE)
        CASE(CS)
        CASE(CC)
        CASE(MI)
        CASE(PL)
        CASE(VS)
        CASE(VC)
        CASE(HI)
        CASE(LS)
        CASE(GE)
        CASE(LT)
        CASE(GT)
        CASE(LE)
        case Condition::AL: {
            // empty
        }
    }

#undef CASE

    return os;
}

uint32_t
eval_shift(ShiftType shift_type, uint32_t value, uint8_t amount, bool& carry) {
    switch (shift_type) {
        case ShiftType::LSL:

            if (amount > 0 && amount <= 32)
                carry = get_nth_bit(value, 32 - amount);
            else if (amount > 32)
                carry = 0;

            return value << amount;
        case ShiftType::LSR:

            if (amount > 0 && amount <= 32)
                carry = get_nth_bit(value, amount - 1);
            else if (amount > 32)
                carry = 0;
            else
                carry = get_nth_bit(value, 31);

            return value >> amount;
        case ShiftType::ASR:
            if (amount > 0 && amount <= 32)
                carry = get_nth_bit(value, amount - 1);
            else
                carry = get_nth_bit(value, 31);

            return static_cast<int32_t>(value) >> amount;
        case ShiftType::ROR:
            if (amount == 0) {
                bool old_carry = carry;

                carry = get_nth_bit(value, 0);
                return (value >> 1) | (old_carry << 31);
            } else {
                carry = get_nth_bit(value, (amount % 32 + 31) % 32);
                return std::rotr(value, amount);
            }
    }
}

std::ostream&
operator<<(std::ostream& os, const ShiftType shift_type) {

#define CASE(type)                                                             \
    case ShiftType::type:                                                      \
        os << #type;                                                           \
        break;

    switch (shift_type) {
        CASE(LSL)
        CASE(LSR)
        CASE(ASR)
        CASE(ROR)
    }

#undef CASE

    return os;
}
