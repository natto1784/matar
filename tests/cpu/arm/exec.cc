#include "cpu/arm/instruction.hh"
#include "cpu/cpu-fixture.hh"
#include "util/bits.hh"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>

using namespace matar;

#define TAG "[arm][execution]"

using namespace arm;

TEST_CASE_METHOD(CpuFixture, "Branch and Exchange", TAG) {
    InstructionData data = BranchAndExchange{ .rn = 3 };

    setr(3, 342800);

    uint32_t cycles = bus->get_cycles();
    exec(data);
    CHECK(bus->get_cycles() == cycles + 3);

    INFO(getr(15));
    INFO(getr(15));
    INFO(getr(15));
    INFO(getr(15));
    // +8 cuz pipeline flush
    CHECK(getr(15) == 342808);
}

TEST_CASE_METHOD(CpuFixture, "Branch", TAG) {
    InstructionData data = Branch{ .link = false, .offset = 3489748 };
    Branch* branch       = std::get_if<Branch>(&data);

    // set PC to 48
    setr(15, 48);

    uint32_t cycles = bus->get_cycles();
    exec(data);
    CHECK(bus->get_cycles() == cycles + 3);

    // 48 + offset
    // +8 cuz pipeline flush
    CHECK(getr(15) == 3489804);
    CHECK(getr(14) == 0);

    // with link
    reset();
    setr(15, 48);
    branch->link = true;
    exec(data);

    // 48 + offset
    // +8 cuz pipeline flush
    CHECK(getr(15) == 3489804);
    // pc was set to 48
    CHECK(getr(14) == 48 - INSTRUCTION_SIZE);
}

TEST_CASE_METHOD(CpuFixture, "Multiply", TAG) {
    InstructionData data = Multiply{
        .rm = 10, .rs = 11, .rn = 3, .rd = 5, .set = false, .acc = false
    };
    Multiply* multiply = std::get_if<Multiply>(&data);

    setr(10, 234912349);
    setr(11, 124897);
    setr(3, 99999); // m = 3 since [32:24] bits are 0

    {
        uint32_t result = 234912349ull * 124897ull & 0xFFFFFFFF;
        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 4); // S + mI

        CHECK(getr(5) == result);
    }

    // with accumulate
    {
        uint32_t result = (234912349ull * 124897ull + 99999ull) & 0xFFFFFFFF;
        multiply->acc   = true;
        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 5); // S + mI + I

        CHECK(getr(5) == result);
    }

    // with set
    {
        uint32_t result = (234912349ull * 124897ull + 99999ull) & 0xFFFFFFFF;
        multiply->set   = true;
        exec(data);

        CHECK(getr(5) == result);
        CHECK(psr().n() == get_bit(result, 31));
    }

    // with set and zero
    {
        setr(10, 0);
        setr(3, 0);
        exec(data);

        CHECK(getr(5) == 0);
        CHECK(psr().n() == false);
        CHECK(psr().z() == true);
    }
}

TEST_CASE_METHOD(CpuFixture, "Multiply Long", TAG) {
    InstructionData data = MultiplyLong{ .rm   = 10,
                                         .rs   = 11,
                                         .rdlo = 3,
                                         .rdhi = 5,
                                         .set  = false,
                                         .acc  = false,
                                         .uns  = true };

    MultiplyLong* multiply_long = std::get_if<MultiplyLong>(&data);

    setr(10, 234912349);
    setr(11, 124897); // m = 3

    // unsigned
    {
        uint64_t result = 234912349ull * 124897ull;
        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 5); // S + (m+1)I

        CHECK(getr(3) == bit_range(result, 0, 31));
        CHECK(getr(5) == bit_range(result, 32, 63));
    }

    // signed
    {
        int64_t result = 234912349ll * -124897ll;
        setr(11, getr(11) * -1);
        multiply_long->uns = false;
        uint32_t cycles    = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 5); // S + (m+1)I

        CHECK(getr(3) == static_cast<uint32_t>(bit_range(result, 0, 31)));
        CHECK(getr(5) == static_cast<uint32_t>(bit_range(result, 32, 63)));
    }

    // accumulate
    {
        setr(3, 99999);
        setr(5, -444333391);

        int64_t result =
          234912349ll * -124897ll + (99999ll | -444333391ll << 32);

        multiply_long->acc = true;
        uint32_t cycles    = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 6); // S + (m+2)I

        CHECK(getr(3) == static_cast<uint32_t>(bit_range(result, 0, 31)));
        CHECK(getr(5) == static_cast<uint32_t>(bit_range(result, 32, 63)));
    }

    // set
    {
        setr(3, 99999);
        setr(5, -444333391);

        int64_t result =
          234912349ll * -124897ll + (99999ll | -444333391ll << 32);

        multiply_long->set = true;
        exec(data);

        CHECK(getr(3) == static_cast<uint32_t>(bit_range(result, 0, 31)));
        CHECK(getr(5) == static_cast<uint32_t>(bit_range(result, 32, 63)));
        CHECK(psr().n() == true);
        CHECK(psr().z() == false);
    }

    // zero
    {
        setr(10, 0);
        setr(5, 0);
        setr(3, 0);

        exec(data);

        CHECK(getr(3) == 0);
        CHECK(getr(5) == 0);
        CHECK(psr().n() == false);
        CHECK(psr().z() == true);
    }
}

TEST_CASE_METHOD(CpuFixture, "Single Data Swap", TAG) {
    InstructionData data =
      SingleDataSwap{ .rm = 3, .rd = 4, .rn = 9, .byte = false };
    SingleDataSwap* swap = std::get_if<SingleDataSwap>(&data);

    setr(9, 0x3003FED);
    setr(3, 94235087);
    setr(3, -259039045);
    bus->write_word(getr(9), 3241011111);

    SECTION("word") {
        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 4); // S + 2N + I

        CHECK(getr(4) == 3241011111);
        CHECK(bus->read_word(getr(9)) == static_cast<uint32_t>(-259039045));
    }

    SECTION("byte") {
        swap->byte = true;
        exec(data);

        CHECK(getr(4) == (3241011111 & 0xFF));
        CHECK(bus->read_byte(getr(9)) ==
              static_cast<uint8_t>(-259039045 & 0xFF));
    }
}

TEST_CASE_METHOD(CpuFixture, "Single Data Transfer", TAG) {
    InstructionData data =
      SingleDataTransfer{ .offset = Shift{ .rm = 3,
                                           .data =
                                             ShiftData{
                                               .type      = ShiftType::ROR,
                                               .immediate = true,
                                               .operand   = 29,
                                             } },
                          .rd     = 5,
                          .rn     = 7,
                          .load   = true,
                          .write  = false,
                          .byte   = false,
                          .up     = true,
                          .pre    = true };
    SingleDataTransfer* data_transfer = std::get_if<SingleDataTransfer>(&data);

    setr(3, 0x63C);
    setr(7, 0x3000004);
    setr(5, -911111);

    // shifted register (immediate)
    {
        // 0x31E + 0x3000004
        bus->write_word(0x30031E4, 95995);
        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 3); // S + N + I

        CHECK(getr(5) == 95995);
        setr(5, 0);
    }

    // shifted register (register)
    {
        data_transfer->offset = Shift{ .rm   = 3,
                                       .data = ShiftData{
                                         .type      = ShiftType::LSL,
                                         .immediate = false,
                                         .operand   = 12,
                                       } };

        setr(12, 2);
        // 6384 + 0x3000004
        bus->write_word(0x30018F4, 3948123487);
        exec(data);

        CHECK(getr(5) == 3948123487);
    }

    // immediate
    {
        data_transfer->offset = static_cast<uint16_t>(0xDA1);
        // 0xDA1 + 0x3000004
        bus->write_word(0x3000DA5, 68795467);

        exec(data);

        CHECK(getr(5) == 68795467);
    }

    // down
    {
        setr(7, 0x3005E0D);
        data_transfer->up = false;
        // 0x3005E0D - 0xDA1
        bus->write_word(0x300506C, 5949595);

        exec(data);

        CHECK(getr(5) == 5949595);
        // no write back
        CHECK(getr(7) == 0x3005E0D);
    }

    // write
    {
        data_transfer->write = true;
        // 0x3005E0D - 0xDA1
        bus->write_word(0x300506C, 967844);

        exec(data);

        CHECK(getr(5) == 967844);
        // 0x3005E0D - 0xDA1
        CHECK(getr(7) == 0x300506C);
    }

    // post
    {
        data_transfer->write = false;
        data_transfer->pre   = false;
        bus->write_word(0x300506C, 61119);

        exec(data);

        CHECK(getr(5) == 61119);
        // 0x300506C - 0xDA1
        CHECK(getr(7) == 0x30042CB);
    }

    // store
    {
        data_transfer->load = false;

        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 2); // 2N for store

        CHECK(bus->read_word(0x30042CB) == 61119);
        // 0x30042CB - 0xDA1
        CHECK(getr(7) == 0x300352A);
    }

    // r15 as rn
    {
        data_transfer->rn = 15;
        setr(15, 0x300352C); // word aligned

        exec(data);

        CHECK(bus->read_word(0x300352C) == 61119);
        // 0x300352C - 0xDA1
        // +4 cuz PC advanced
        // and then word aligned
        CHECK(getr(15) == 0x300278C);

        // cleanup
        data_transfer->rn = 7;
    }

    // r15 as rd
    {
        data_transfer->rd = 15;
        setr(15, 444444);

        exec(data);

        CHECK(bus->read_word(0x300352A) == 444444 + 4);
        // 0x300352A - 0xDA1
        CHECK(getr(7) == 0x3002789);

        // cleanup
        data_transfer->rd = 5;
    }

    // byte
    {
        data_transfer->byte = true;

        setr(5, 458267584);

        exec(data);

        CHECK(bus->read_word(0x3002789) == (458267584 & 0xFF));
        // 0x3002789 - 0xDA1
        CHECK(getr(7) == 0x30019E8);
    }

    // r15 as rd with load
    {
        data_transfer->rd   = 15;
        data_transfer->load = true;
        setr(15, 0);
        bus->write_byte(0x30019E8, 0xE2);

        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() ==
              cycles + 5); // 2S + 2N + I for load with rd=15

        // +8 cuz pipeline flushed then word aligned
        // so +6
        CHECK(getr(15) == 0xE8);

        // 0x30019E8 - 0xDA1
        CHECK(getr(7) == 0x3000C47);

        // cleanup
        data_transfer->rd = 5;
    }
}

TEST_CASE_METHOD(CpuFixture, "Halfword Transfer", TAG) {
    InstructionData data          = HalfwordTransfer{ .offset = 12,
                                                      .half   = true,
                                                      .sign   = false,
                                                      .rd     = 11,
                                                      .rn     = 10,
                                                      .load   = true,
                                                      .write  = false,
                                                      .imm    = false,
                                                      .up     = true,
                                                      .pre    = true };
    HalfwordTransfer* hw_transfer = std::get_if<HalfwordTransfer>(&data);

    setr(12, 0x384);
    setr(11, 459058287);
    setr(10, 0x300611E);

    // register offset
    {
        //  0x300611E  + 0x384
        bus->write_word(0x30064A2, 3948123487);

        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 3); // S + N + I

        CHECK(getr(11) == (3948123487 & 0xFFFF));
    }

    // immediate offset
    {
        hw_transfer->imm    = true;
        hw_transfer->offset = 0xA7;
        // 0x300611E + 0xA7
        bus->write_word(0x30061C5, 594633302);
        exec(data);

        CHECK(getr(11) == (594633302 & 0xFFFF));
    }

    // down
    {
        hw_transfer->up = false;
        // 0x300611E - 0xA7
        bus->write_word(0x3006077, 222221);

        exec(data);

        CHECK(getr(11) == (222221 & 0xFFFF));
        // no write back
        CHECK(getr(10) == 0x300611E);
    }

    // write
    {
        hw_transfer->write = true;
        // 0x300611E - 0xA7
        bus->write_word(0x3006077, 100000005);

        exec(data);

        CHECK(getr(11) == (100000005 & 0xFFFF));
        CHECK(getr(10) == 0x3006077);
    }

    // post
    {
        hw_transfer->pre   = false;
        hw_transfer->write = false;
        bus->write_word(0x3006077, 6111909);

        exec(data);

        CHECK(getr(11) == (6111909 & 0xFFFF));
        // 0x3006077 - 0xA7
        CHECK(getr(10) == 0x3005FD0);
    }

    // store
    {
        hw_transfer->load = false;

        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 2); // 2N

        CHECK(bus->read_halfword(0x3005FD0) == (6111909 & 0xFFFF));
        // 0x3005FD0 - 0xA7
        CHECK(getr(10) == 0x3005F29);
    }

    // r15 as rn
    {
        hw_transfer->rn = 15;
        setr(15, 0x3005F28); // word aligned

        exec(data);

        CHECK(bus->read_halfword(0x3005F28) == (6111909 & 0xFFFF));
        // 0x3005F28 - 0xA7
        // +4 cuz PC advanced
        // and then word aligned
        CHECK(getr(15) == 0x3005E84);

        // cleanup
        hw_transfer->rn = 10;
    }

    // r15 as rd
    {
        hw_transfer->rd = 15;
        setr(15, 224);

        exec(data);

        CHECK(bus->read_halfword(0x3005F29) == 224 + 4);
        // 0x3005F29 - 0xA7
        CHECK(getr(10) == 0x3005E82);

        // cleanup
        hw_transfer->rd = 11;
    }

    // signed halfword
    {
        hw_transfer->load = true;
        hw_transfer->sign = true;
        bus->write_halfword(0x3005E82, -12345);

        exec(data);

        CHECK(getr(11) == static_cast<uint32_t>(-12345));
        // 0x3005E82 - 0xA7
        CHECK(getr(10) == 0x3005DDB);
    }

    // signed byte
    {
        hw_transfer->half = false;
        bus->write_byte(0x3005DDB, -56);

        exec(data);

        CHECK(getr(11) == static_cast<uint32_t>(-56));
        // 0x3005DDB - 0xA7
        CHECK(getr(10) == 0x3005D34);
    }

    // r15 as rd with load
    {
        hw_transfer->rd   = 15;
        hw_transfer->load = true;
        setr(15, 0);
        bus->write_byte(0x3005D34, 56);

        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() ==
              cycles + 5); // 2S + 2N + I for load with rd=15

        // +8 cuz pipeline flushed then word aligned
        CHECK(getr(15) == static_cast<uint32_t>(56 + 8));

        // 0x3005D34 - 0xA7
        CHECK(getr(10) == 0x3005C8D);

        // cleanup
        hw_transfer->rd = 11;
    }
}

TEST_CASE_METHOD(CpuFixture, "Block Data Transfer", TAG) {
    InstructionData data = BlockDataTransfer{ .regs  = 0b1010100111000001,
                                              .rn    = 10,
                                              .load  = true,
                                              .write = false,
                                              .s     = false,
                                              .up    = true,
                                              .pre   = true };

    BlockDataTransfer* block_transfer  = std::get_if<BlockDataTransfer>(&data);
    static constexpr uint8_t alignment = 4;

    // load
    SECTION("load") {
        static constexpr uint32_t address = 0x3000D78;
        // populate memory
        bus->write_word(address, 38947234);
        bus->write_word(address + alignment, 237164);
        bus->write_word(address + alignment * 2, 679785111);
        bus->write_word(address + alignment * 3, 905895898);
        bus->write_word(address + alignment * 4, 131313333);
        bus->write_word(address + alignment * 5, 131);
        bus->write_word(address + alignment * 6, 989231);
        bus->write_word(address + alignment * 7, 6);

        auto checker = [this](uint32_t rnval = 0) {
            CHECK(getr(0) == 237164);
            CHECK(getr(1) == 0);
            CHECK(getr(2) == 0);
            CHECK(getr(3) == 0);
            CHECK(getr(4) == 0);
            CHECK(getr(5) == 0);
            CHECK(getr(6) == 679785111);
            CHECK(getr(7) == 905895898);
            CHECK(getr(8) == 131313333);
            CHECK(getr(9) == 0);
            CHECK(getr(10) == rnval);
            CHECK(getr(11) == 131);
            CHECK(getr(12) == 0);
            CHECK(getr(13) == 989231);
            CHECK(getr(14) == 0);

            // setting r15 as 6, flushes the pipeline causing it to go 6 + 8
            // i.e, 14. word aligning this, gives us 12
            CHECK(getr(15) == 12);

            for (uint8_t i = 0; i < 16; i++) {
                setr(i, 0);
            }
        };

        setr(10, address);
        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 11); // (n+1)S + 2N + I
        checker(address);

        // with write
        setr(10, address);
        block_transfer->write = true;
        exec(data);
        checker(address + 7 * alignment);

        // decrement
        block_transfer->write = false;
        block_transfer->up    = false;
        // adjust rn
        setr(10, address + alignment * 8);
        exec(data);
        checker(address + alignment * 8);

        // with write
        setr(10, address + alignment * 8);
        block_transfer->write = true;
        exec(data);
        checker(address + alignment);

        // post increment
        block_transfer->write = false;
        block_transfer->up    = true;
        block_transfer->pre   = false;
        // adjust rn
        setr(10, address + alignment);
        exec(data);
        checker(address + alignment * 8);

        // post decrement
        block_transfer->up = false;
        // adjust rn
        setr(10, address + alignment * 7);
        exec(data);
        checker(address);

        // with s bit
        cpu.chg_mode(Mode::Fiq);
        block_transfer->s = true;
        CHECK(psr().raw() != psr(true).raw());
        exec(data);
        CHECK(psr().raw() == psr(true).raw());
    }

    // store
    SECTION("store") {
        static constexpr uint32_t address = 0x30015A8;

        block_transfer->load = false;

        // populate registers
        setr(0, 237164);
        setr(6, 679785111);
        setr(7, 905895898);
        setr(8, 131313333);
        setr(11, 131);
        setr(13, 989231);
        setr(15, 4); // word align

        // we will count the number of steps to count PC advances
        uint8_t steps = 0;

        auto checker = [this, &steps]() {
            CHECK(bus->read_word(address + alignment) == 237164);
            CHECK(bus->read_word(address + alignment * 2) == 679785111);
            CHECK(bus->read_word(address + alignment * 3) == 905895898);
            CHECK(bus->read_word(address + alignment * 4) == 131313333);
            CHECK(bus->read_word(address + alignment * 5) == 131);
            CHECK(bus->read_word(address + alignment * 6) == 989231);
            CHECK(bus->read_word(address + alignment * 7) ==
                  4 + (4 * (steps - 1)));

            for (uint8_t i = 1; i < 8; i++)
                bus->write_word(address + alignment * i, 0);
        };

        setr(10, address); // base
        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 8); // 2N + (n-1)S
        steps++;
        checker();

        // decrement
        block_transfer->write = false;
        block_transfer->up    = false;
        // adjust rn
        setr(10, address + alignment * 8);
        exec(data);
        steps++;
        checker();

        // post increment
        block_transfer->up  = true;
        block_transfer->pre = false;
        // adjust rn
        setr(10, address + alignment);
        exec(data);
        steps++;
        checker();

        // post decrement
        block_transfer->up = false;
        // adjust rn
        setr(10, address + alignment * 7);
        exec(data);
        steps++;
        checker();

        // with s bit
        cpu.chg_mode(Mode::Fiq);
        block_transfer->s = true;
        exec(data);
        steps++;
        // User's R13 is different (unset at this point)
        CHECK(bus->read_word(address + alignment * 6) == 0);
    }
}

TEST_CASE_METHOD(CpuFixture, "PSR Transfer", TAG) {
    InstructionData data = PsrTransfer{
        .operand = 12,
        .spsr    = false,
        .type    = PsrTransfer::Type::Mrs,
        .imm     = false,
    };
    PsrTransfer* psr_transfer = std::get_if<PsrTransfer>(&data);

    SECTION("MRS") {
        setr(12, 12389398);

        CHECK(psr().raw() != getr(12));
        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 1); // 1S
        CHECK(psr().raw() == getr(12));

        psr_transfer->spsr = true;
        // with SPSR
        CHECK(psr(true).raw() != getr(12));
        exec(data);
        CHECK(psr(true).raw() == getr(12));
    }

    // MSR
    SECTION("MSR") {
        psr_transfer->type = PsrTransfer::Type::Msr;
        // go to the reserved bits
        setr(12, 16556u << 8);

        CHECK(psr().raw() != getr(12));
        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 1); // 1S
        CHECK(psr().raw() == getr(12));

        psr_transfer->spsr = true;
        // with SPSR
        CHECK(psr(true).raw() != getr(12));
        exec(data);
        CHECK(psr(true).raw() == getr(12));
    }

    // MSR_flg
    SECTION("MSR_flg") {
        psr_transfer->type = PsrTransfer::Type::Msr_flg;

        setr(12, 1490352945);
        // go to the reserved bits

        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 1); // 1S
        CHECK(psr().n() == get_bit(1490352945, 31));
        CHECK(psr().z() == get_bit(1490352945, 30));
        CHECK(psr().c() == get_bit(1490352945, 29));
        CHECK(psr().v() == get_bit(1490352945, 28));

        // with SPSR and immediate operand
        psr_transfer->operand = 99333394;
        psr_transfer->imm     = true;
        psr_transfer->spsr    = true;
        exec(data);
        CHECK(psr().n() == get_bit(1490352945, 31));
        CHECK(psr(true).n() == get_bit(9933394, 31));
        CHECK(psr(true).z() == get_bit(9933394, 30));
        CHECK(psr(true).c() == get_bit(9933394, 29));
        CHECK(psr(true).v() == get_bit(9933394, 28));
    }
}

TEST_CASE_METHOD(CpuFixture, "Data Processing", TAG) {
    using OpCode = DataProcessing::OpCode;

    InstructionData data =
      DataProcessing{ .operand = Shift{ .rm = 3,
                                        .data =
                                          ShiftData{
                                            .type      = ShiftType::ROR,
                                            .immediate = true,
                                            .operand   = 29,
                                          } },
                      .rd      = 5,
                      .rn      = 7,
                      .set     = true,
                      .opcode  = OpCode::AND };
    DataProcessing* processing = std::get_if<DataProcessing>(&data);

    // operand 1
    setr(7, -28717);

    // AND with shifted register (imediate)
    {
        // rm
        setr(3, 1596);
        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 1); // 1S
        // -28717 & 12768
        CHECK(getr(5) == 448);
    }

    // AND with shifted register (register)
    {
        processing->operand = Shift{ .rm   = 3,
                                     .data = ShiftData{
                                       .type      = ShiftType::LSL,
                                       .immediate = false,
                                       .operand   = 12,
                                     } };
        // rm
        setr(3, 1596);
        // rs
        setr(12, 2);

        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 2); // 1S + 1I

        // -28717 & 6384
        CHECK(getr(5) == 2256);
    }

    // same as above but with rn (oprerand 1) = 15
    {
        processing->rn = 15;
        setr(15, -2871);
        exec(data);

        // (-2871 + INSTRUCTION_SIZE) & 6384
        CHECK(getr(5) == ((-2871 + INSTRUCTION_SIZE) & 6384));

        // cleanup
        processing->rn = 7;
    }

    auto reset_flags = [this]() {
        Psr cpsr = psr();
        cpsr.set_n(false);
        cpsr.set_z(false);
        cpsr.set_v(false);
        cpsr.set_c(false);
        set_psr(cpsr);
    };

    auto flags = [this, reset_flags](bool n, bool z, bool v, bool c) {
        CHECK(psr().n() == n);
        CHECK(psr().z() == z);
        CHECK(psr().v() == v);
        CHECK(psr().c() == c);
        reset_flags();
    };

    // immediate operand
    processing->operand = static_cast<uint32_t>(54924809);
    // rs
    setr(12, 2);
    setr(5, 0);
    reset_flags();

    SECTION("AND (with condition check)") {
        Psr cpsr = psr();

        processing->opcode = OpCode::AND;

        exec(data, Condition::EQ);

        // condition is false
        CHECK(getr(5) == 0);

        cpsr.set_z(true);
        set_psr(cpsr);
        exec(data, Condition::EQ);
        exec(data, Condition::EQ);

        // -28717 & 54924809
        // condition is true now
        CHECK(getr(5) == 54920705);

        // check set flags
        flags(false, false, false, false);
    }

    // TST with immediate operand
    SECTION("TST") {
        processing->opcode = OpCode::TST;

        exec(data);

        // -28717 & 54924809
        CHECK(getr(5) == 0);

        // check set flags
        flags(false, false, false, false);
    }

    SECTION("EOR (with condition check)") {
        Psr cpsr = psr();

        processing->opcode = OpCode::EOR;

        cpsr.set_c(true);
        set_psr(cpsr);
        exec(data, Condition::CC);

        // condition fails
        CHECK(getr(5) == 0);

        cpsr = psr();
        cpsr.set_c(false);
        set_psr(cpsr);
        exec(data, Condition::CC);

        // -28717 ^ 54924809
        // condition is true now
        CHECK(getr(5) == 4240021978);

        // check set flags
        flags(true, false, false, false);

        // check zero flag
        processing->operand = static_cast<uint32_t>(-28717);
        exec(data);

        // -28717 ^ -28717
        CHECK(getr(5) == 0);

        // check set flags
        flags(false, true, false, false);
    }

    SECTION("TEQ") {
        processing->opcode = OpCode::TEQ;

        exec(data);

        // -28717 ^ 54924809
        CHECK(getr(5) == 0);

        // check set flags
        flags(true, false, false, false);
    }

    SECTION("SUB") {
        processing->opcode = OpCode::SUB;
        exec(data);

        // -28717 - 54924809
        CHECK(getr(5) == static_cast<uint32_t>(-54953526));

        // check set flags
        flags(true, false, false, true);

        // check zero flag
        processing->operand = static_cast<uint32_t>(-28717);
        exec(data);

        // -28717 - (-28717)
        CHECK(getr(5) == 0);

        // check set flags
        flags(false, true, false, true);
    }

    SECTION("CMP") {
        processing->opcode = OpCode::CMP;

        exec(data);

        // -28717 - 54924809
        CHECK(getr(5) == 0);

        // check set flags
        flags(true, false, false, true);
    }

    SECTION("RSB") {
        processing->opcode = OpCode::RSB;
        exec(data);

        // +28717 + 54924809
        CHECK(getr(5) == 54953526);

        // check set flags
        flags(false, false, false, false);
    }

    SECTION("ADD") {
        processing->opcode = OpCode::ADD;
        exec(data);

        // -28717 + 54924809
        CHECK(getr(5) == 54896092);

        // check set flags
        flags(false, false, false, false);

        // test zero flag
        processing->operand = static_cast<uint32_t>(28717);
        exec(data);

        // -28717 + 28717
        CHECK(getr(5) == 0);

        // check set flags
        flags(false, true, false, false);

        // test overflow flag
        processing->operand = static_cast<uint32_t>((1u << 31) - 1);
        setr(7, (1u << 31) - 1);

        exec(data);

        CHECK(getr(5) == (1ull << 32) - 2);

        // check set flags
        flags(true, false, true, false);
    }

    SECTION("CMN") {
        processing->opcode = OpCode::CMN;

        exec(data);

        // -28717 + 54924809
        CHECK(getr(5) == 0);

        // check set flags
        flags(false, false, false, false);
    }

    SECTION("ADC") {
        Psr cpsr           = psr();
        processing->opcode = OpCode::ADC;

        cpsr.set_c(true);
        set_psr(cpsr);
        exec(data);

        // -28717 + 54924809 + carry
        CHECK(getr(5) == 54896093);

        // check set flags
        flags(false, false, false, false);
    }

    SECTION("SBC") {
        Psr cpsr           = psr();
        processing->opcode = OpCode::SBC;

        cpsr.set_c(false);
        set_psr(cpsr);
        exec(data);

        // -28717 - 54924809 + carry - 1
        CHECK(getr(5) == static_cast<uint32_t>(-54953527));

        // check set flags
        flags(true, false, false, false);
    }

    SECTION("RSC") {
        Psr cpsr           = psr();
        processing->opcode = OpCode::RSC;

        cpsr.set_c(false);
        set_psr(cpsr);
        exec(data);

        // +28717 +54924809 + carry - 1
        CHECK(getr(5) == 54953525);

        // check set flags
        flags(false, false, false, false);
    }

    SECTION("ORR") {
        processing->opcode = OpCode::ORR;
        exec(data);

        // -28717 | 54924809
        CHECK(getr(5) == static_cast<uint32_t>(-24613));

        // check set flags
        flags(true, false, false, false);
    }

    SECTION("BIC") {
        processing->opcode = OpCode::BIC;
        exec(data);

        // -28717 & ~54924809
        CHECK(getr(5) == static_cast<uint32_t>(-54949422));

        // check set flags
        flags(true, false, false, false);
    }

    SECTION("MVN") {
        processing->opcode = OpCode::MVN;
        exec(data);

        // ~54924809
        CHECK(getr(5) == static_cast<uint32_t>(-54924810));

        // check set flags
        flags(true, false, false, false);
    }

    SECTION("R15 as destination") {
        processing->opcode = OpCode::MVN;
        processing->rd     = 15;
        setr(15, 0);
        CHECK(psr(true).raw() != psr().raw());
        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 3); // 2S + N

        // ~54924809 + 8 (for flush) and then word adjust
        CHECK(getr(15) == static_cast<uint32_t>(-54924804));

        // flags are not set
        flags(false, false, false, false);
        CHECK(psr(true).raw() == psr().raw());
    }
}

#undef TAG
