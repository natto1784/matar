#include "bus.hh"
#include <catch2/catch_test_macros.hpp>

#define TAG "[bus]"

using namespace matar;

class BusFixture {
  public:
    BusFixture()
      : bus(Memory(std::array<uint8_t, Memory::BIOS_SIZE>(),
                   std::vector<uint8_t>(Header::HEADER_SIZE))) {}

  protected:
    Bus bus;
};

TEST_CASE_METHOD(BusFixture, "Byte", TAG) {
    CHECK(bus.read_byte(0x30001A9) == 0);

    bus.write_byte(0x30001A9, 0xEC);
    CHECK(bus.read_byte(0x30001A9) == 0xEC);
    CHECK(bus.read_word(0x30001A9) == 0xEC);
    CHECK(bus.read_halfword(0x30001A9) == 0xEC);
}

TEST_CASE_METHOD(BusFixture, "Halfword", TAG) {
    CHECK(bus.read_halfword(0x202FED9) == 0);

    bus.write_halfword(0x202FED9, 0x1A4A);
    CHECK(bus.read_halfword(0x202FED9) == 0x1A4A);
    CHECK(bus.read_word(0x202FED9) == 0x1A4A);
    CHECK(bus.read_byte(0x202FED9) == 0x4A);
}

TEST_CASE_METHOD(BusFixture, "Word", TAG) {
    CHECK(bus.read_word(0x600EE34) == 0);

    bus.write_word(0x600EE34, 0x3ACC491D);
    CHECK(bus.read_word(0x600EE34) == 0x3ACC491D);
    CHECK(bus.read_halfword(0x600EE34) == 0x491D);
    CHECK(bus.read_byte(0x600EE34) == 0x1D);
}

#undef TAG
