#include "utility.hh"
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

std::ostream&
operator<<(std::ostream& os, const OpCode opcode) {

#define CASE(opcode)                                                           \
    case OpCode::opcode:                                                       \
        os << #opcode;                                                         \
        break;

    switch (opcode) {
        CASE(AND)
        CASE(EOR)
        CASE(SUB)
        CASE(RSB)
        CASE(ADD)
        CASE(ADC)
        CASE(SBC)
        CASE(RSC)
        CASE(TST)
        CASE(TEQ)
        CASE(CMP)
        CASE(CMN)
        CASE(ORR)
        CASE(MOV)
        CASE(BIC)
        CASE(MVN)
    }

#undef CASE

    return os;
}

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
