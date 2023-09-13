#include "utility.hh"

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
