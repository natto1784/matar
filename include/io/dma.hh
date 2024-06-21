#include <bit>
#include <cstdint>

namespace matar {
// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays)
struct DmaControl {
    struct {
        int : 4; // this is supposed to be 5 bits, however, to align the struct
                 // to 16 bits, we will adjust for the first LSB in the
                 // read/write
        uint8_t dst_adjustment : 2;
        uint8_t src_adjustment : 2;
        bool repeat : 1;
        bool transfer_32 : 1;
        int : 1;
        uint8_t start_timing : 2;
        bool irq_enable : 1;
        bool enable : 1;
    } value;

    uint16_t read() const { return std::bit_cast<uint16_t>(value) << 1; };
    void write(uint16_t raw) {
        value = std::bit_cast<decltype(value)>(static_cast<uint16_t>(raw >> 1));
    };
};

struct Dma {
    using u16 = uint16_t;

    struct {
        u16 source[2];
        u16 destination[2];
        u16 word_count;
        DmaControl control;
    } channels[4];
};
// NOLINTEND(cppcoreguidelines-avoid-c-arrays)

}
