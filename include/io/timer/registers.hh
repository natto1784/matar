#pragma once

#include <bit>

namespace matar {
struct TimerControl {
    struct {
        uint8_t prescaler : 2;
        bool count_up : 1;
        int : 3;
        bool irq_enable : 1;
        bool start_stop : 1;
        int : 8;
    } value;

    uint16_t read() const { return std::bit_cast<uint16_t>(value); };
    void write(uint16_t raw) { value = std::bit_cast<decltype(value)>(raw); };
};
}
