#include "bus.hh"
#include <catch2/catch_test_macros.hpp>

#define TAG "[bus]"

using namespace matar;

class BusFixture {
  public:
    BusFixture()
      : bus(Bus::init(std::array<uint8_t, Bus::BIOS_SIZE>(),
                      std::vector<uint8_t>(Header::HEADER_SIZE))) {}

  protected:
    std::shared_ptr<Bus> bus;
};

TEST_CASE("bios", TAG) {
    std::array<uint8_t, Bus::BIOS_SIZE> bios = { 0 };

    // populate bios
    bios[0]      = 0xAC;
    bios[0x3FFF] = 0x48;
    bios[0x2A56] = 0x10;

    auto bus =
      Bus::init(std::move(bios), std::vector<uint8_t>(Header::HEADER_SIZE));

    uint32_t cycles = bus->get_cycles();

    CHECK(bus->read_byte(0) == 0xAC);
    CHECK(bus->read_byte(0x3FFF) == 0x48);
    CHECK(bus->read_byte(0x2A56) == 0x10);

    CHECK(bus->get_cycles() == cycles + 3);
}

TEST_CASE_METHOD(BusFixture, "board wram", TAG) {
    uint32_t cycles = bus->get_cycles();

    bus->write_byte(0x2000000, 0xAC);
    CHECK(bus->read_byte(0x2000000) == 0xAC);

    bus->write_byte(0x203FFFF, 0x48);
    CHECK(bus->read_byte(0x203FFFF) == 0x48);

    bus->write_byte(0x2022A56, 0x10);
    CHECK(bus->read_byte(0x2022A56) == 0x10);

    CHECK(bus->get_cycles() == cycles + 2 * 9);
    cycles = bus->get_cycles();

    bus->write_halfword(0x2022A56, 0x1009);
    CHECK(bus->read_halfword(0x2022A56) == 0x1009);

    CHECK(bus->get_cycles() == cycles + 2 * 3);
    cycles = bus->get_cycles();

    bus->write_word(0x2022A56, 0x10FF9903);
    CHECK(bus->read_word(0x2022A56) == 0x10FF9903);

    CHECK(bus->get_cycles() == cycles + 2 * 6);
}

TEST_CASE_METHOD(BusFixture, "chip wram", TAG) {
    uint32_t cycles = bus->get_cycles();

    bus->write_byte(0x3000000, 0xAC);
    CHECK(bus->read_byte(0x3000000) == 0xAC);

    bus->write_byte(0x3007FFF, 0x48);
    CHECK(bus->read_byte(0x3007FFF) == 0x48);

    bus->write_byte(0x3002A56, 0x10);
    CHECK(bus->read_byte(0x3002A56) == 0x10);

    CHECK(bus->get_cycles() == cycles + 2 * 3);
    cycles = bus->get_cycles();

    bus->write_halfword(0x3002A56, 0xF0F0);
    CHECK(bus->read_halfword(0x3002A56) == 0xF0F0);

    CHECK(bus->get_cycles() == cycles + 2);
    cycles = bus->get_cycles();

    bus->write_word(0x3002A56, 0xF9399010);
    CHECK(bus->read_word(0x3002A56) == 0xF9399010);

    CHECK(bus->get_cycles() == cycles + 2);
}

TEST_CASE_METHOD(BusFixture, "palette ram", TAG) {
    uint32_t cycles = bus->get_cycles();

    bus->write_byte(0x5000000, 0xAC);
    CHECK(bus->read_byte(0x5000000) == 0xAC);

    bus->write_byte(0x50003FF, 0x48);
    CHECK(bus->read_byte(0x50003FF) == 0x48);

    bus->write_byte(0x5000156, 0x10);
    CHECK(bus->read_byte(0x5000156) == 0x10);

    CHECK(bus->get_cycles() == cycles + 2 * 3);
    cycles = bus->get_cycles();

    bus->write_halfword(0x5000156, 0xEEE1);
    CHECK(bus->read_halfword(0x5000156) == 0xEEE1);

    CHECK(bus->get_cycles() == cycles + 2);
    cycles = bus->get_cycles();

    bus->write_word(0x5000156, 0x938566E0);
    CHECK(bus->read_word(0x5000156) == 0x938566E0);

    CHECK(bus->get_cycles() == cycles + 2 * 2);
}

TEST_CASE_METHOD(BusFixture, "video ram", TAG) {
    uint32_t cycles = bus->get_cycles();

    bus->write_byte(0x6000000, 0xAC);
    CHECK(bus->read_byte(0x6000000) == 0xAC);

    bus->write_byte(0x6017FFF, 0x48);
    CHECK(bus->read_byte(0x6017FFF) == 0x48);

    bus->write_byte(0x6012A56, 0x10);
    CHECK(bus->read_byte(0x6012A56) == 0x10);

    CHECK(bus->get_cycles() == cycles + 2 * 3);
    cycles = bus->get_cycles();

    bus->write_halfword(0x6012A56, 0xB100);
    CHECK(bus->read_halfword(0x6012A56) == 0xB100);

    CHECK(bus->get_cycles() == cycles + 2);
    cycles = bus->get_cycles();

    bus->write_word(0x6012A56, 0x9322093E);
    CHECK(bus->read_word(0x6012A56) == 0x9322093E);

    CHECK(bus->get_cycles() == cycles + 2 * 2);
}

TEST_CASE_METHOD(BusFixture, "oam obj ram", TAG) {
    uint32_t cycles = bus->get_cycles();

    bus->write_byte(0x7000000, 0xAC);
    CHECK(bus->read_byte(0x7000000) == 0xAC);

    bus->write_byte(0x70003FF, 0x48);
    CHECK(bus->read_byte(0x70003FF) == 0x48);

    bus->write_byte(0x7000156, 0x10);
    CHECK(bus->read_byte(0x7000156) == 0x10);

    CHECK(bus->get_cycles() == cycles + 2 * 3);
    cycles = bus->get_cycles();

    bus->write_halfword(0x7000156, 0x946C);
    CHECK(bus->read_halfword(0x7000156) == 0x946C);

    CHECK(bus->get_cycles() == cycles + 2);
    cycles = bus->get_cycles();

    bus->write_word(0x7000156, 0x93C5D1E0);
    CHECK(bus->read_word(0x7000156) == 0x93C5D1E0);

    CHECK(bus->get_cycles() == cycles + 2);
}

TEST_CASE("rom", TAG) {
    std::vector<uint8_t> rom(32 * 1024 * 1024, 0);

    // populate rom
    rom[0]         = 0xAC;
    rom[0x1FFFFFF] = 0x48;
    rom[0x0EF0256] = 0x10;

    // 32 megabyte ROM
    auto bus = Bus::init(std::array<uint8_t, Bus::BIOS_SIZE>(), std::move(rom));

    SECTION("ROM1") {
        CHECK(bus->read_byte(0x8000000) == 0xAC);
        CHECK(bus->read_byte(0x9FFFFFF) == 0x48);
        CHECK(bus->read_byte(0x8EF0256) == 0x10);
    }

    SECTION("ROM2") {
        CHECK(bus->read_byte(0xA000000) == 0xAC);
        CHECK(bus->read_byte(0xBFFFFFF) == 0x48);
        CHECK(bus->read_byte(0xAEF0256) == 0x10);
    }

    SECTION("ROM3") {
        CHECK(bus->read_byte(0xC000000) == 0xAC);
        CHECK(bus->read_byte(0xDFFFFFF) == 0x48);
        CHECK(bus->read_byte(0xCEF0256) == 0x10);
    }
}

TEST_CASE_METHOD(BusFixture, "internal cycle", TAG) {
    uint32_t cycles = bus->get_cycles();

    bus->internal_cycle();
    bus->internal_cycle();

    CHECK(bus->get_cycles() == cycles + 2);
}

#undef TAG
