#include "bus.hh"
#include "util/log.hh"
#include <memory>

namespace matar {
Bus::Bus(const Memory& memory)
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
    if (address & 0b01)
        glogger.warn("Reading a non aligned halfword address");

    return memory->read(address) | memory->read(address + 1) << 8;
}

void
Bus::write_halfword(size_t address, uint16_t halfword) {
    if (address & 0b01)
        glogger.warn("Writing to a non aligned halfword address");

    memory->write(address, halfword & 0xFF);
    memory->write(address + 1, halfword >> 8 & 0xFF);
}

uint32_t
Bus::read_word(size_t address) {
    if (address & 0b11)
        glogger.warn("Reading a non aligned word address");

    return memory->read(address) | memory->read(address + 1) << 8 |
           memory->read(address + 2) << 16 | memory->read(address + 3) << 24;
}

void
Bus::write_word(size_t address, uint32_t word) {
    if (address & 0b11)
        glogger.warn("Writing to a non aligned word address");

    memory->write(address, word & 0xFF);
    memory->write(address + 1, word >> 8 & 0xFF);
    memory->write(address + 2, word >> 16 & 0xFF);
    memory->write(address + 3, word >> 24 & 0xFF);
}
}
