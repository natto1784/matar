#pragma once

#include <cstdint>

namespace matar {
enum class Mode {
    /* M[4:0] in PSR */
    User       = 0b10000,
    Fiq        = 0b10001,
    Irq        = 0b10010,
    Supervisor = 0b10011,
    Abort      = 0b10111,
    Undefined  = 0b11011,
    System     = 0b11111,
};

enum class State {
    Arm   = 0,
    Thumb = 1
};

enum class Condition {
    EQ = 0b0000,
    NE = 0b0001,
    CS = 0b0010,
    CC = 0b0011,
    MI = 0b0100,
    PL = 0b0101,
    VS = 0b0110,
    VC = 0b0111,
    HI = 0b1000,
    LS = 0b1001,
    GE = 0b1010,
    LT = 0b1011,
    GT = 0b1100,
    LE = 0b1101,
    AL = 0b1110
};

constexpr auto
stringify(Condition cond) {

#define CASE(cond)                                                             \
    case Condition::cond:                                                      \
        return #cond;

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
            return "";
        }
    }

#undef CASE

    return "";
}

class Psr {
  public:
    Psr() = default;
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

    uint32_t psr;
};
}
