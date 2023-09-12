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
