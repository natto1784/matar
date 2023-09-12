#include "bus.hh"
#include <memory>

Bus::Bus(Memory& memory)
  : memory(std::make_shared<Memory>(memory)) {}

uint8_t
Bus::read_byte(size_t address) {
    return memory->read(address);
}

void
Bus::write_byte(size_t address, uint8_t byte) {
    memory->write(address, byte);
}

uint16_t
Bus::read_halfword(size_t address) {
    return memory->read_halfword(address);
}

void
Bus::write_halfword(size_t address, uint16_t halfword) {
    memory->write_halfword(address, halfword);
}

uint32_t
Bus::read_word(size_t address) {
    return memory->read_word(address);
}

void
Bus::write_word(size_t address, uint32_t word) {
    memory->write_halfword(address, word);
}
