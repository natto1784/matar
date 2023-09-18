#pragma once

#include "utility.hh"
#include <cstdint>

class Psr {
  public:
    // clear the reserved bits i.e, [8:27]
    Psr(uint32_t raw);

    uint32_t raw() const;
    void set_all(uint32_t raw);

    // Mode : [4:0]
    Mode mode() const;
    void set_mode(Mode mode);

    // State : [5]
    State state() const;
    void set_state(State state);

#define GET_SET_NTH_BIT_FUNCTIONS(name)                                        \
    bool name() const;                                                         \
    void set_##name(bool val);

    // FIQ disable : [6]
    GET_SET_NTH_BIT_FUNCTIONS(fiq_disabled)

    // IRQ disable : [7]
    GET_SET_NTH_BIT_FUNCTIONS(irq_disabled)

    // Reserved bits : [27:8]

    // Overflow flag : [28]
    GET_SET_NTH_BIT_FUNCTIONS(v)

    // Carry flag : [29]
    GET_SET_NTH_BIT_FUNCTIONS(c)

    // Zero flag : [30]
    GET_SET_NTH_BIT_FUNCTIONS(z)

    // Negative flag : [30]
    GET_SET_NTH_BIT_FUNCTIONS(n)

#undef GET_SET_NTH_BIT_FUNCTIONS

    bool condition(Condition cond) const;

  private:
    static constexpr uint32_t PSR_CLEAR_RESERVED = 0xF00000FF;
    static constexpr uint32_t PSR_CLEAR_MODE     = 0xFFFFFFE0;

    uint32_t psr;
};
