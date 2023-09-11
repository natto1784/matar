#include "psr.hh"
#include "util/bits.hh"

Psr::Psr(uint32_t raw) {
    psr = raw & PSR_CLEAR_RESERVED;
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

bool
Psr::state() const {
    return get_nth_bit(psr, 5);
}

void
Psr::set_state(State state) {
    chg_nth_bit(psr, 5, static_cast<bool>(state));
}

#define GET_SET_NTH_BIT_FUNCTIONS(name, n)                                     \
    bool Psr::name() const {                                                   \
        return get_nth_bit(psr, n);                                            \
    }                                                                          \
    void Psr::set_##name(bool val) {                                           \
        chg_nth_bit(psr, n, val);                                              \
    }

GET_SET_NTH_BIT_FUNCTIONS(fiq_disabled, 6)

GET_SET_NTH_BIT_FUNCTIONS(irq_disabled, 7)

GET_SET_NTH_BIT_FUNCTIONS(v, 28);

GET_SET_NTH_BIT_FUNCTIONS(c, 29);

GET_SET_NTH_BIT_FUNCTIONS(z, 30);

GET_SET_NTH_BIT_FUNCTIONS(n, 31);

#undef GET_SET_NTH_BIT_FUNCTIONS
