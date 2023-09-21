#pragma once

#include "memory.hh"
#include <memory>

namespace matar {
class Bus {
  public:
    Bus(const Memory& memory);

    uint8_t read_byte(size_t address);
    void write_byte(size_t address, uint8_t byte);

    uint16_t read_halfword(size_t address);
    void write_halfword(size_t address, uint16_t halfword);

    uint32_t read_word(size_t address);
    void write_word(size_t address, uint32_t word);

  private:
    std::shared_ptr<Memory> memory;
};
}
