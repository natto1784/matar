#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

// ill use [] instead of at because i dont want if (...) throw conditions for
// all accesses to improve performance (?)

// we are also not gonna perform bound checks, as i expect the user to handle
// those

namespace matar {
template<std::size_t N = 0>
class Memory {
    // we can use either a vector or an array with this
    using Container = std::
      conditional_t<(N != 0), std::array<uint8_t, N>, std::vector<uint8_t>>;

  public:
    Memory() = default;
    Memory(auto x)
      : memory(x) {}

    uint8_t read_byte(std::size_t idx) const { return memory[idx]; }

    void write_byte(std::size_t idx, uint8_t byte) { memory[idx] = byte; }

    uint16_t read_halfword(std::size_t idx) const {
        return memory[idx] | memory[idx + 1] << 8;
    }

    void write_halfword(std::size_t idx, uint16_t halfword) {
        memory[idx]     = halfword & 0xFF;
        memory[idx + 1] = halfword >> 8 & 0xFF;
    }

    uint32_t read_word(std::size_t idx) const {
        return memory[idx] | memory[idx + 1] << 8 | memory[idx + 2] << 16 |
               memory[idx + 3] << 24;
    }

    void write_word(std::size_t idx, uint32_t word) {
        memory[idx]     = word & 0xFF;
        memory[idx + 1] = word >> 8 & 0xFF;
        memory[idx + 2] = word >> 16 & 0xFF;
        memory[idx + 3] = word >> 24 & 0xFF;
    }

    uint8_t& operator[](std::size_t idx) { return memory.at(idx); }

    Container& data() { return memory; }

    std::size_t size() const { return memory.size(); }

  private:
    Container memory;
};
}
