#include "psr.hh"
#include "util/bits.hh"
#include "util/log.hh"

Psr::Psr(uint32_t raw)
  : psr(raw & PSR_CLEAR_RESERVED) {}

uint32_t
Psr::raw() const {
    return psr;
}

void
Psr::set_all(uint32_t raw) {
    psr = raw & ~PSR_CLEAR_RESERVED;
}

Mode
Psr::mode() const {
    return static_cast<Mode>(psr & ~PSR_CLEAR_MODE);
}

void
Psr::set_mode(Mode mode) {
    psr &= PSR_CLEAR_MODE;
    psr |= static_cast<uint32_t>(mode);
}

State
Psr::state() const {
    return static_cast<State>(get_bit(psr, 5));
}

void
Psr::set_state(State state) {
    chg_bit(psr, 5, static_cast<bool>(state));
}

#define GET_SET_NTH_BIT_FUNCTIONS(name, n)                                     \
    bool Psr::name() const {                                                   \
        return get_bit(psr, n);                                                \
    }                                                                          \
    void Psr::set_##name(bool val) {                                           \
        chg_bit(psr, n, val);                                                  \
    }

GET_SET_NTH_BIT_FUNCTIONS(fiq_disabled, 6)

GET_SET_NTH_BIT_FUNCTIONS(irq_disabled, 7)

GET_SET_NTH_BIT_FUNCTIONS(v, 28);

GET_SET_NTH_BIT_FUNCTIONS(c, 29);

GET_SET_NTH_BIT_FUNCTIONS(z, 30);

GET_SET_NTH_BIT_FUNCTIONS(n, 31);

#undef GET_SET_NTH_BIT_FUNCTIONS

bool
Psr::condition(Condition cond) const {
    switch (cond) {
        case Condition::EQ:
            return z();
        case Condition::NE:
            return !z();
        case Condition::CS:
            return c();
        case Condition::CC:
            return !c();
        case Condition::MI:
            return n();
        case Condition::PL:
            return !n();
        case Condition::VS:
            return v();
        case Condition::VC:
            return !v();
        case Condition::HI:
            return c() && !z();
        case Condition::LS:
            return !c() || z();
        case Condition::GE:
            return n() == v();
        case Condition::LT:
            return n() != v();
        case Condition::GT:
            return !z() && (n() == v());
        case Condition::LE:
            return z() || (n() != v());
        case Condition::AL:
            return true;
    }

    return false;
}
