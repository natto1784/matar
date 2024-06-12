#include "bus.hh"
#include "util/log.hh"
#include <memory>

namespace matar {

static constexpr uint32_t IO_START = 0x4000000;
static constexpr uint32_t IO_END   = 0x40003FE;

Bus::Bus(const Memory& memory)
  : memory(std::make_shared<Memory>(memory)) {}

uint8_t
Bus::read_byte(uint32_t address) {
    if (address >= IO_START && address <= IO_END)
        return io.read_byte(address);

    return memory->read(address);
}

void
Bus::write_byte(uint32_t address, uint8_t byte) {
    if (address >= IO_START && address <= IO_END) {
        io.write_byte(address, byte);
        return;
    }

    memory->write(address, byte);
}

uint16_t
Bus::read_halfword(uint32_t address) {
    if (address & 0b01)
        glogger.warn("Reading a non aligned halfword address");

    if (address >= IO_START && address <= IO_END)
        return io.read_halfword(address);

    return read_byte(address) | read_byte(address + 1) << 8;
}

void
Bus::write_halfword(uint32_t address, uint16_t halfword) {
    if (address & 0b01)
        glogger.warn("Writing to a non aligned halfword address");

    if (address >= IO_START && address <= IO_END) {
        io.write_halfword(address, halfword);
        return;
    }

    write_byte(address, halfword & 0xFF);
    write_byte(address + 1, halfword >> 8 & 0xFF);
}

uint32_t
Bus::read_word(uint32_t address) {
    if (address & 0b11)
        glogger.warn("Reading a non aligned word address");

    if (address >= IO_START && address <= IO_END)
        return io.read_word(address);

    return read_byte(address) | read_byte(address + 1) << 8 |
           read_byte(address + 2) << 16 | read_byte(address + 3) << 24;
}

void
Bus::write_word(uint32_t address, uint32_t word) {
    if (address & 0b11)
        glogger.warn("Writing to a non aligned word address");

    if (address >= IO_START && address <= IO_END) {
        io.write_word(address, word);
        return;
    }

    write_byte(address, word & 0xFF);
    write_byte(address + 1, word >> 8 & 0xFF);
    write_byte(address + 2, word >> 16 & 0xFF);
    write_byte(address + 3, word >> 24 & 0xFF);
}
}
