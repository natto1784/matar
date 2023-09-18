#include "cpu/cpu.hh"
#include "cpu/utility.hh"
#include <bit>
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <limits>
#include <random>

// I could have written some public API but that wouldn't be the best practice,
// so instead I will try to do my best to test these functions using memory
// manipulation. We also use a fake PC to match the current instruction's
// address.
//
// We are going to use some addresses for specific tasks
// - (4 * 400) + 4 => Storing, then reading registers
//
// We are also going to keep some registers reserved for testing
// - R0 is always zero
// - R1 for reading PSR

class CpuFixture {
  public:
    uint32_t fake_pc = 2 * ARM_INSTRUCTION_SIZE;

    CpuFixture()
      // BIOS is all zeroes so let's do what we can
      : memory(std::array<uint8_t, Memory::BIOS_SIZE>(),
               std::vector<uint8_t>(192))
      , bus(memory)
      , cpu(bus) {}

    void write_register(uint8_t rd, uint8_t value, uint8_t rotate = 0) {
        // MOV
        uint32_t raw = 0b11100011101000000000000000000000;
        raw |= rd << 12;
        raw |= rotate << 8;
        raw |= value;
        execute(raw);
    }

    uint32_t read_register(uint8_t rd) {
        // use R0
        static constexpr uint16_t offset = MAX_FAKE_PC + ARM_INSTRUCTION_SIZE;

        uint32_t raw = 0b11100101100000000000000000000000;
        raw |= rd << 12;
        raw |= offset;
        execute(raw);

        return bus.read_word(offset + (rd == 15 ? ARM_INSTRUCTION_SIZE : 0));
    }

    Psr read_cpsr() {
        // use R1
        uint32_t raw = 0b11100001000011110001000000000000;
        execute(raw);

        return Psr(read_register(1));
    }

    void execute(uint32_t raw) {
        bus.write_word(fake_pc - 2 * ARM_INSTRUCTION_SIZE, raw);
        step();
    }

  private:
    static constexpr uint32_t MAX_FAKE_PC = 400 * ARM_INSTRUCTION_SIZE;
    Memory memory;

    void step() {
        cpu.step();
        fake_pc += ARM_INSTRUCTION_SIZE;
        if (fake_pc == MAX_FAKE_PC)
            fake_pc = 0;
    }

  protected:
    Bus bus;
    Cpu cpu;
};

#define TAG "arm execution"

using namespace arm;

TEST_CASE_METHOD(CpuFixture, "Test fixture", TAG) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> value_d;
    std::uniform_int_distribution<uint8_t> shift_d(0, (1 << 4) - 1);

    // R0 is reserved to be 0 so that it can be used as as offset
    write_register(0, 0);
    REQUIRE(read_register(0) == 0);

    for (uint8_t i = 1; i < 15; i++) {
        uint8_t value   = value_d(gen);
        uint8_t shift   = shift_d(gen);
        uint32_t amount = std::rotr(static_cast<uint32_t>(value), 2 * shift);

        write_register(i, value, shift);
        REQUIRE(read_register(i) == amount);
    }

    REQUIRE(read_cpsr().mode() == Mode::Supervisor);

    INFO("Fixture is OK");
}

TEST_CASE_METHOD(CpuFixture, "Branch and Exchange", TAG) {
    uint32_t raw = 0b11100001001011111111111100011010;

    write_register(10, 240);

    execute(raw);
    fake_pc = 240 + 2 * ARM_INSTRUCTION_SIZE;

    REQUIRE(read_register(15) == 240 + 2 * ARM_INSTRUCTION_SIZE);
}

// TODO write BX for when switching to thumb

TEST_CASE_METHOD(CpuFixture, "Branch", TAG) {
    uint32_t raw = 0b11101011000000000000000000111100;

    uint32_t old_pc = fake_pc;
    execute(raw);
    fake_pc = old_pc + 240;

    // pipeline is flushed
    fake_pc += 2 * ARM_INSTRUCTION_SIZE;

    REQUIRE(read_register(15) == old_pc + 240 + 2 * ARM_INSTRUCTION_SIZE);
    REQUIRE(read_register(14) == old_pc - ARM_INSTRUCTION_SIZE);
}

TEST_CASE_METHOD(CpuFixture, "Multiply", TAG) {
    uint32_t raw    = 0b11100000001111011100101110011010;
    uint32_t result = 0;

    write_register(10, 230);
    write_register(11, 192);
    write_register(12, 37);

    execute(raw);
    result = 230 * 192 + 37;

    REQUIRE(read_register(13) == result);
    REQUIRE(read_cpsr().n() == (result >> 31 & 1));

    // when product is zero
    write_register(10, 230);
    write_register(11, 0);
    write_register(12, 0);

    execute(raw);

    REQUIRE(read_register(13) == 0);
    REQUIRE(read_cpsr().z() == true);
}

TEST_CASE_METHOD(CpuFixture, "Multiply Long", TAG) {
    uint32_t raw    = 0b11100000101111011100101110011010;
    uint64_t result = 0;

    write_register(10, 230, 3);  // 2550136835
    write_register(11, 192, 12); // 49152
    write_register(12, 255, 9);  // 4177920
    write_register(13, 11, 4);   // 184549376

    result = 2550136835ull * 49152ull + (184549376ull << 32 | 4177920ull);

    execute(raw);

    REQUIRE(read_register(12) == (result & 0xFFFFFFFF));
    REQUIRE(read_register(13) == (result >> 32 & 0xFFFFFFFF));
    REQUIRE(read_cpsr().z() == false);
    REQUIRE(read_cpsr().n() == (result >> 63 & 1));

    // signed
    raw = 0b11100000111111011100101110011010;

    write_register(12, 255, 9); // 4177920
    write_register(13, 11, 4);  // 184549376

    execute(raw);

    REQUIRE(read_register(12) == (result & 0xFFFFFFFF));
    REQUIRE(read_register(13) == (result >> 32 & 0xFFFFFFFF));
    REQUIRE(read_cpsr().z() == false);
    REQUIRE(read_cpsr().n() == (result >> 63 & 1));

    // 0 and no accumulation
    raw = 0b11100000110111011100101110011010;

    write_register(10, 0);
    execute(raw);

    REQUIRE(read_register(12) == 0);
    REQUIRE(read_register(13) == 0);
    REQUIRE(read_cpsr().z() == true);
}

TEST_CASE_METHOD(CpuFixture, "Single Data Swap", TAG) {
    write_register(6, 230, 3); // 2550136835
    write_register(9, 160, 0); // 160
    bus.write_word(read_register(9), 49152);

    SECTION("word") {
        uint32_t raw = 0b11100001000010010101000010010110;
        execute(raw);

        REQUIRE(read_register(5) == 49152);
        REQUIRE(bus.read_word(read_register(9)) == 2550136835);
    }

    SECTION("byte") {
        uint32_t raw = 0b11100001010010010101000010010110;

        execute(raw);

        REQUIRE(read_register(5) == (49152 & 0xFF));
        REQUIRE(bus.read_byte(read_register(9)) == (2550136835 & 0xFF));
    }
}

#undef TAG
