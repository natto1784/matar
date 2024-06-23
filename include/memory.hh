#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

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
        uint16_t val;
        std::memcpy(&val, &memory[idx], 2);
        return val;
    }

    void write_halfword(std::size_t idx, uint16_t halfword) {
        std::memcpy(&memory[idx], &halfword, 2);
    }

    uint32_t read_word(std::size_t idx) const {
        uint32_t val;
        std::memcpy(&val, &memory[idx], 4);
        return val;
    }

    void write_word(std::size_t idx, uint32_t word) {
        std::memcpy(&memory[idx], &word, 4);
    }

    uint8_t& operator[](std::size_t idx) { return memory.at(idx); }

    Container& data() { return memory; }

    constexpr std::size_t size() const { return memory.size(); }

  private:
    Container memory;
};
}
