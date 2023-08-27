#pragma once

#include "bits.hh"
#include "utility.hh"
#include <cstdint>

static constexpr uint32_t PSR_CLEAR_RESERVED = 0xf00000ff;
static constexpr uint32_t PSR_CLEAR_MODE     = 0x0b00000;

class Psr {
    uint32_t psr;

  public:
    // clear the reserved bits i.e, [8:27]
    Psr(uint32_t raw) { psr = raw & PSR_CLEAR_RESERVED; }

    // Mode : [4:0]
    Mode mode() const { return static_cast<Mode>(psr & ~PSR_CLEAR_MODE); }
    void set_mode(Mode mode) {
        psr &= PSR_CLEAR_MODE;
        psr |= static_cast<uint32_t>(mode);
    }

    // State : [5]
    bool state() const { return get_nth_bit(psr, 5); }
    void set_state(State state) {
        chg_nth_bit(psr, 5, static_cast<bool>(state));
    }

#define GET_SET_NTH_BIT_FUNCTIONS(name, n)                                     \
    bool name() const { return get_nth_bit(psr, n); }                          \
    void set_##name(bool val) { chg_nth_bit(psr, n, val); }

    // FIQ disable : [6]
    GET_SET_NTH_BIT_FUNCTIONS(fiq_disabled, 6)

    // IRQ disable : [7]
    GET_SET_NTH_BIT_FUNCTIONS(irq_disabled, 7)

    // Reserved bits : [27:8]

    // Overflow flag : [28]
    GET_SET_NTH_BIT_FUNCTIONS(v, 28);

    // Carry flag : [29]
    GET_SET_NTH_BIT_FUNCTIONS(c, 29);

    // Zero flag : [30]
    GET_SET_NTH_BIT_FUNCTIONS(z, 30);

    // Negative flag : [30]
    GET_SET_NTH_BIT_FUNCTIONS(n, 31);

#undef GET_SET_NTH_BIT_FUNCTIONS
};
