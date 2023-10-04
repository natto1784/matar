#include "memory.hh"
#include <catch2/catch_test_macros.hpp>

#define TAG "[memory]"

using namespace matar;

class MemFixture {
  public:
    MemFixture()
      : memory(std::array<uint8_t, Memory::BIOS_SIZE>(),
               std::vector<uint8_t>(Header::HEADER_SIZE)) {}

  protected:
    Memory memory;
};

TEST_CASE("bios", TAG) {
    std::array<uint8_t, Memory::BIOS_SIZE> bios = { 0 };

    // populate bios
    bios[0]      = 0xAC;
    bios[0x3FFF] = 0x48;
    bios[0x2A56] = 0x10;

    Memory memory(std::move(bios), std::vector<uint8_t>(Header::HEADER_SIZE));

    CHECK(memory.read(0) == 0xAC);
    CHECK(memory.read(0x3FFF) == 0x48);
    CHECK(memory.read(0x2A56) == 0x10);
}

TEST_CASE_METHOD(MemFixture, "board wram", TAG) {
    memory.write(0x2000000, 0xAC);
    CHECK(memory.read(0x2000000) == 0xAC);

    memory.write(0x203FFFF, 0x48);
    CHECK(memory.read(0x203FFFF) == 0x48);

    memory.write(0x2022A56, 0x10);
    CHECK(memory.read(0x2022A56) == 0x10);
}

TEST_CASE_METHOD(MemFixture, "chip wram", TAG) {
    memory.write(0x3000000, 0xAC);
    CHECK(memory.read(0x3000000) == 0xAC);

    memory.write(0x3007FFF, 0x48);
    CHECK(memory.read(0x3007FFF) == 0x48);

    memory.write(0x3002A56, 0x10);
    CHECK(memory.read(0x3002A56) == 0x10);
}

TEST_CASE_METHOD(MemFixture, "palette ram", TAG) {
    memory.write(0x5000000, 0xAC);
    CHECK(memory.read(0x5000000) == 0xAC);

    memory.write(0x50003FF, 0x48);
    CHECK(memory.read(0x50003FF) == 0x48);

    memory.write(0x5000156, 0x10);
    CHECK(memory.read(0x5000156) == 0x10);
}

TEST_CASE_METHOD(MemFixture, "video ram", TAG) {
    memory.write(0x6000000, 0xAC);
    CHECK(memory.read(0x6000000) == 0xAC);

    memory.write(0x6017FFF, 0x48);
    CHECK(memory.read(0x6017FFF) == 0x48);

    memory.write(0x6012A56, 0x10);
    CHECK(memory.read(0x6012A56) == 0x10);
}

TEST_CASE_METHOD(MemFixture, "oam obj ram", TAG) {
    memory.write(0x7000000, 0xAC);
    CHECK(memory.read(0x7000000) == 0xAC);

    memory.write(0x70003FF, 0x48);
    CHECK(memory.read(0x70003FF) == 0x48);

    memory.write(0x7000156, 0x10);
    CHECK(memory.read(0x7000156) == 0x10);
}

TEST_CASE("rom", TAG) {
    std::vector<uint8_t> rom(32 * 1024 * 1024, 0);

    // populate rom
    rom[0]         = 0xAC;
    rom[0x1FFFFFF] = 0x48;
    rom[0x0EF0256] = 0x10;

    // 32 megabyte ROM
    Memory memory(std::array<uint8_t, Memory::BIOS_SIZE>(), std::move(rom));

    SECTION("ROM1") {
        CHECK(memory.read(0x8000000) == 0xAC);
        CHECK(memory.read(0x9FFFFFF) == 0x48);
        CHECK(memory.read(0x8EF0256) == 0x10);
    }

    SECTION("ROM2") {
        CHECK(memory.read(0xA000000) == 0xAC);
        CHECK(memory.read(0xBFFFFFF) == 0x48);
        CHECK(memory.read(0xAEF0256) == 0x10);
    }

    SECTION("ROM3") {
        CHECK(memory.read(0xC000000) == 0xAC);
        CHECK(memory.read(0xDFFFFFF) == 0x48);
        CHECK(memory.read(0xCEF0256) == 0x10);
    }
}

#undef TAG
