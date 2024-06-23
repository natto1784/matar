#pragma once

#include <bit>

namespace matar {
struct WaitstateControl {
    using u8 = uint8_t;

    struct {
        using u8 = uint8_t;
        u8 sram_wait_control : 2;
        u8 wait_state_0_first : 2;
        u8 wait_state_0_second : 1;
        u8 wait_state_1_first : 2;
        u8 wait_state_1_second : 1;
        u8 wait_state_2_first : 2;
        u8 wait_state_2_second : 1;
        u8 phi_terminal_output : 2;
        int : 1;
        bool gamepak_prefetch_buffer : 1;
        bool gamepak_type_flag : 1;
    } value;

    uint16_t read() const { return std::bit_cast<uint16_t>(value); };
    void write(uint16_t raw) { value = std::bit_cast<decltype(value)>(raw); };
};
}
