#pragma once

#include <bit>
#include <cstdint>

namespace matar {
struct DmaControl {
    enum class Timing {
        Immediately = 0,
        VBlank      = 1,
        HBlank      = 2,
        Special     = 3
    };

    struct {
        /* this is supposed to be 5 bits, however, to align the struct to 16
         * bits, we will adjust for the first LSB in the read/write */
        int : 4;
        uint8_t dst_ctrl : 2;
        uint8_t src_ctrl : 2;
        bool repeat : 1;
        bool transfer_32 : 1;
        int : 1;
        uint8_t timing : 2;
        bool irq_enable : 1;
        bool enable : 1;
    } value;

    uint16_t read() const { return std::bit_cast<uint16_t>(value) << 1; };
    void write(uint16_t raw) {
        value = std::bit_cast<decltype(value)>(static_cast<uint16_t>(raw >> 1));
    };
};
}
