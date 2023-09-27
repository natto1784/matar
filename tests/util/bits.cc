#include "util/bits.hh"
#include <catch2/catch_test_macros.hpp>

#define TAG "[util][bits]"

TEST_CASE("8 bits", TAG) {
    uint8_t num = 45;

    CHECK(get_bit(num, 0));
    CHECK(!get_bit(num, 1));
    CHECK(get_bit(num, 5));
    CHECK(!get_bit(num, 6));
    CHECK(!get_bit(num, 7));

    set_bit(num, 6);
    CHECK(get_bit(num, 6));

    rst_bit(num, 6);
    CHECK(!get_bit(num, 6));

    chg_bit(num, 5, false);
    CHECK(!get_bit(num, 5));

    chg_bit(num, 5, true);
    CHECK(get_bit(num, 5));

    // 0b0110
    CHECK(bit_range(num, 1, 4) == 6);
}

TEST_CASE("16 bits", TAG) {
    uint16_t num = 34587;

    CHECK(get_bit(num, 0));
    CHECK(get_bit(num, 1));
    CHECK(!get_bit(num, 5));
    CHECK(!get_bit(num, 14));
    CHECK(get_bit(num, 15));

    set_bit(num, 14);
    CHECK(get_bit(num, 14));

    rst_bit(num, 14);
    CHECK(!get_bit(num, 14));

    chg_bit(num, 5, true);
    CHECK(get_bit(num, 5));

    // num = 45
    chg_bit(num, 5, false);
    CHECK(!get_bit(num, 5));

    // 0b1000110
    CHECK(bit_range(num, 2, 8) == 70);
}

TEST_CASE("32 bits", TAG) {
    uint32_t num = 3194142523;

    CHECK(get_bit(num, 0));
    CHECK(get_bit(num, 1));
    CHECK(get_bit(num, 12));
    CHECK(get_bit(num, 29));
    CHECK(!get_bit(num, 30));
    CHECK(get_bit(num, 31));

    set_bit(num, 30);
    CHECK(get_bit(num, 30));

    rst_bit(num, 30);
    CHECK(!get_bit(num, 30));

    chg_bit(num, 12, false);
    CHECK(!get_bit(num, 12));

    chg_bit(num, 12, true);
    CHECK(get_bit(num, 12));

    // 0b10011000101011111100111
    CHECK(bit_range(num, 3, 25) == 5003239);
}

TEST_CASE("64 bits", TAG) {
    uint64_t num = 58943208889991935;

    CHECK(get_bit(num, 0));
    CHECK(get_bit(num, 1));
    CHECK(!get_bit(num, 10));
    CHECK(get_bit(num, 55));
    CHECK(!get_bit(num, 60));

    set_bit(num, 63);
    CHECK(get_bit(num, 63));

    rst_bit(num, 63);
    CHECK(!get_bit(num, 63));

    chg_bit(num, 10, true);
    CHECK(get_bit(num, 10));

    chg_bit(num, 10, false);
    CHECK(!get_bit(num, 10));

    // 0b011010001
    CHECK(bit_range(num, 39, 47) == 209);
}

#undef TAG
