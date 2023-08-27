#pragma once

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
