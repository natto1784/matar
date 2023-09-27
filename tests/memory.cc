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

TEST_CASE_METHOD(MemFixture, "bios", TAG) {
    memory.write(0, 0xAC);
    CHECK(memory.read(0) == 0xAC);

    memory.write(0x3FFF, 0x48);
    CHECK(memory.read(0x3FFF) == 0x48);

    memory.write(0x2A56, 0x10);
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
    // 32 megabyte ROM
    Memory memory(std::array<uint8_t, Memory::BIOS_SIZE>(),
                  std::vector<uint8_t>(32 * 1024 * 1024));

    SECTION("ROM1") {
        memory.write(0x8000000, 0xAC);
        CHECK(memory.read(0x8000000) == 0xAC);

        memory.write(0x9FFFFFF, 0x48);
        CHECK(memory.read(0x9FFFFFF) == 0x48);

        memory.write(0x8ef0256, 0x10);
        CHECK(memory.read(0x8ef0256) == 0x10);
    }

    SECTION("ROM2") {
        memory.write(0xA000000, 0xAC);
        CHECK(memory.read(0xA000000) == 0xAC);

        memory.write(0xBFFFFFF, 0x48);
        CHECK(memory.read(0xBFFFFFF) == 0x48);

        memory.write(0xAEF0256, 0x10);
        CHECK(memory.read(0xAEF0256) == 0x10);
    }

    SECTION("ROM3") {
        memory.write(0xC000000, 0xAC);
        CHECK(memory.read(0xC000000) == 0xAC);

        memory.write(0xDFFFFFF, 0x48);
        CHECK(memory.read(0xDFFFFFF) == 0x48);

        memory.write(0xCEF0256, 0x10);
        CHECK(memory.read(0xCEF0256) == 0x10);
    }
}

#undef TAG
