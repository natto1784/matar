#include "cpu/cpu-fixture.hh"
#include "cpu/cpu-impl.hh"
#include "util/bits.hh"
#include <catch2/catch_test_macros.hpp>

using namespace matar;

#define TAG "[arm][execution]"

using namespace arm;

TEST_CASE_METHOD(CpuFixture, "Branch and Exchange", TAG) {
    InstructionData data = BranchAndExchange{ .rn = 3 };

    setr(3, 342800);

    exec(data);

    CHECK(getr(15) == 342800);
}

TEST_CASE_METHOD(CpuFixture, "Branch", TAG) {
    InstructionData data = Branch{ .link = false, .offset = 3489748 };
    Branch* branch       = std::get_if<Branch>(&data);

    exec(data);

    CHECK(getr(15) == 3489748);
    CHECK(getr(14) == 0);

    // with link
    reset();
    branch->link = true;
    exec(data);

    CHECK(getr(15) == 3489748);
    CHECK(getr(14) == 0 + INSTRUCTION_SIZE);
}

TEST_CASE_METHOD(CpuFixture, "Multiply", TAG) {
    InstructionData data = Multiply{
        .rm = 10, .rs = 11, .rn = 3, .rd = 5, .set = false, .acc = false
    };
    Multiply* multiply = std::get_if<Multiply>(&data);

    setr(10, 234912349);
    setr(11, 124897);
    setr(3, 99999);

    {
        uint32_t result = 234912349ull * 124897ull & 0xFFFFFFFF;
        exec(data);

        CHECK(getr(5) == result);
    }

    // with accumulate
    {
        uint32_t result = (234912349ull * 124897ull + 99999ull) & 0xFFFFFFFF;
        multiply->acc   = true;
        exec(data);

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
    setr(11, 124897);

    // unsigned
    {
        uint64_t result = 234912349ull * 124897ull;
        exec(data);

        CHECK(getr(3) == bit_range(result, 0, 31));
        CHECK(getr(5) == bit_range(result, 32, 63));
    }

    // signed
    {
        int64_t result = 234912349ll * -124897ll;
        setr(11, getr(11) * -1);
        multiply_long->uns = false;
        exec(data);

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
        exec(data);

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

    setr(9, 0x3FED);
    setr(3, 94235087);
    setr(3, -259039045);
    bus.write_word(getr(9), 3241011111);

    SECTION("word") {
        exec(data);

        CHECK(getr(4) == 3241011111);
        CHECK(bus.read_word(getr(9)) == static_cast<uint32_t>(-259039045));
    }

    SECTION("byte") {
        swap->byte = true;
        exec(data);

        CHECK(getr(4) == (3241011111 & 0xFF));
        CHECK(bus.read_byte(getr(9)) ==
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

    setr(3, 1596);
    setr(7, 6);
    setr(5, -911111);

    // shifted register (immediate)
    {
        // 12768 + 6
        bus.write_word(12774, 95995);
        exec(data);

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
        // 6384 + 6
        bus.write_word(6390, 3948123487);
        exec(data);

        CHECK(getr(5) == 3948123487);
    }

    // immediate
    {
        data_transfer->offset = static_cast<uint16_t>(3489);
        // 6 + 3489
        bus.write_word(3495, 68795467);

        exec(data);

        CHECK(getr(5) == 68795467);
    }

    // down
    {
        setr(7, 18044);
        data_transfer->up = false;
        // 18044 - 3489
        bus.write_word(14555, 5949595);

        exec(data);

        CHECK(getr(5) == 5949595);
        // no write back
        CHECK(getr(7) == 18044);
    }

    // write
    {
        data_transfer->write = true;
        bus.write_word(14555, 967844);

        exec(data);

        CHECK(getr(5) == 967844);
        // 18044 - 3489
        CHECK(getr(7) == 14555);
    }

    // post
    {
        data_transfer->write = false;
        data_transfer->pre   = false;
        bus.write_word(14555, 61119);

        exec(data);

        CHECK(getr(5) == 61119);
        // 14555 - 3489
        CHECK(getr(7) == 11066);
    }

    // store
    {
        data_transfer->load = false;

        exec(data);

        CHECK(bus.read_word(11066) == 61119);
        // 11066 - 3489
        CHECK(getr(7) == 7577);
    }

    // r15 as rn
    {
        data_transfer->rn = 15;
        setr(15, 7577);

        exec(data);

        CHECK(bus.read_word(7577 - 2 * INSTRUCTION_SIZE) == 61119);
        // 7577 - 3489
        CHECK(getr(15) == 4088 - 2 * INSTRUCTION_SIZE);

        // cleanup
        data_transfer->rn = 7;
    }

    // r15 as rd
    {
        // 4088
        data_transfer->rd = 15;
        setr(15, 444444);

        exec(data);

        CHECK(bus.read_word(7577 + INSTRUCTION_SIZE) == 444444);
        // 7577 - 3489
        CHECK(getr(7) == 4088 + INSTRUCTION_SIZE);

        // cleanup
        data_transfer->rd = 5;
        setr(7, getr(7) - INSTRUCTION_SIZE);
    }

    // byte
    {
        data_transfer->byte = true;

        setr(5, 458267584);

        exec(data);

        CHECK(bus.read_word(4088) == (458267584 & 0xFF));
        // 4088 - 3489
        CHECK(getr(7) == 599);
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

    setr(12, 8404);
    setr(11, 459058287);
    setr(10, 900);

    // register offset
    {
        // 900 + 8404
        bus.write_word(9304, 3948123487);
        exec(data);

        CHECK(getr(11) == (3948123487 & 0xFFFF));
    }

    // immediate offset
    {
        hw_transfer->imm    = true;
        hw_transfer->offset = 167;
        // 900 + 167
        bus.write_word(1067, 594633302);
        exec(data);

        CHECK(getr(11) == (594633302 & 0xFFFF));
    }

    // down
    {
        hw_transfer->up = false;
        // 900 - 167
        bus.write_word(733, 222221);

        exec(data);

        CHECK(getr(11) == (222221 & 0xFFFF));
        // no write back
        CHECK(getr(10) == 900);
    }

    // write
    {
        hw_transfer->write = true;
        // 900 - 167
        bus.write_word(733, 100000005);

        exec(data);

        CHECK(getr(11) == (100000005 & 0xFFFF));
        // 900 - 167
        CHECK(getr(10) == 733);
    }

    // post
    {
        hw_transfer->pre   = false;
        hw_transfer->write = false;
        bus.write_word(733, 6111909);

        exec(data);

        CHECK(getr(11) == (6111909 & 0xFFFF));
        // 733 - 167
        CHECK(getr(10) == 566);
    }

    // store
    {
        hw_transfer->load = false;

        exec(data);

        CHECK(bus.read_halfword(566) == (6111909 & 0xFFFF));
        // 566 - 167
        CHECK(getr(10) == 399);
    }

    // r15 as rn
    {
        hw_transfer->rn = 15;
        setr(15, 399);

        exec(data);

        CHECK(bus.read_halfword(399 - 2 * INSTRUCTION_SIZE) ==
              (6111909 & 0xFFFF));
        // 399 - 167
        CHECK(getr(15) == 232 - 2 * INSTRUCTION_SIZE);

        // cleanup
        hw_transfer->rn = 10;
    }

    // r15 as rd
    {
        hw_transfer->rd = 15;
        setr(15, 224);

        exec(data);

        CHECK(bus.read_halfword(399 + INSTRUCTION_SIZE) == 224);
        // 399 - 167
        CHECK(getr(10) == 232 + INSTRUCTION_SIZE);

        // cleanup
        hw_transfer->rd = 11;
        setr(10, 399);
    }

    // signed halfword
    {
        hw_transfer->load = true;
        hw_transfer->sign = true;
        bus.write_halfword(399, -12345);

        exec(data);

        CHECK(getr(11) == static_cast<uint32_t>(-12345));
        // 399 - 167
        CHECK(getr(10) == 232);
    }

    // signed byte
    {
        hw_transfer->half = false;
        bus.write_byte(232, -56);

        exec(data);

        CHECK(getr(11) == static_cast<uint32_t>(-56));
        // 232 - 167
        CHECK(getr(10) == 65);
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

    BlockDataTransfer* block_transfer = std::get_if<BlockDataTransfer>(&data);

    // load
    SECTION("load") {
        // populate memory
        bus.write_word(3448, 38947234);
        bus.write_word(3452, 237164);
        bus.write_word(3456, 679785111);
        bus.write_word(3460, 905895898);
        bus.write_word(3464, 131313333);
        bus.write_word(3468, 131);
        bus.write_word(3472, 989231);
        bus.write_word(3476, 6);

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
            CHECK(getr(15) == 6);

            for (uint8_t i = 0; i < 16; i++) {
                setr(i, 0);
            }
        };

        setr(10, 3448);
        exec(data);
        checker(3448);

        // with write
        setr(10, 3448);
        block_transfer->write = true;
        exec(data);
        checker(3448 + INSTRUCTION_SIZE);

        // decrement
        block_transfer->write = false;
        block_transfer->up    = false;
        // adjust rn
        setr(10, 3480);
        exec(data);
        checker(3480);

        // with write
        setr(10, 3480);
        block_transfer->write = true;
        exec(data);
        checker(3480 - INSTRUCTION_SIZE);

        // post increment
        block_transfer->write = false;
        block_transfer->up    = true;
        block_transfer->pre   = false;
        // adjust rn
        setr(10, 3452);
        exec(data);
        checker(3452 + INSTRUCTION_SIZE);

        // post decrement
        block_transfer->up = false;
        // adjust rn
        setr(10, 3476);
        exec(data);
        checker(3476 - INSTRUCTION_SIZE);

        // with s bit
        cpu.chg_mode(Mode::Fiq);
        block_transfer->s = true;
        CHECK(psr().raw() != psr(true).raw());
        exec(data);
        CHECK(psr().raw() == psr(true).raw());
    }

    // store
    SECTION("store") {
        block_transfer->load = false;

        // populate registers
        setr(0, 237164);
        setr(6, 679785111);
        setr(7, 905895898);
        setr(8, 131313333);
        setr(11, 131);
        setr(13, 989231);
        setr(15, 6);

        auto checker = [this]() {
            CHECK(bus.read_word(5548) == 237164);
            CHECK(bus.read_word(5552) == 679785111);
            CHECK(bus.read_word(5556) == 905895898);
            CHECK(bus.read_word(5560) == 131313333);
            CHECK(bus.read_word(5564) == 131);
            CHECK(bus.read_word(5568) == 989231);
            CHECK(bus.read_word(5572) == 6);

            for (uint8_t i = 0; i < 8; i++)
                bus.write_word(5548 + i * 4, 0);
        };

        setr(10, 5544); // base
        exec(data);
        checker();

        // decrement
        block_transfer->write = false;
        block_transfer->up    = false;
        // adjust rn
        setr(10, 5576);
        exec(data);
        checker();

        // post increment
        block_transfer->up  = true;
        block_transfer->pre = false;
        // adjust rn
        setr(10, 5548);
        exec(data);
        checker();

        // post decrement
        block_transfer->up = false;
        // adjust rn
        setr(10, 5572);
        exec(data);
        checker();

        // with s bit
        cpu.chg_mode(Mode::Fiq);
        block_transfer->s = true;
        cpu.chg_mode(Mode::Supervisor);
        // User's R13 is different (unset at this point)
        CHECK(bus.read_word(5568) == 0);
        exec(data);
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
        exec(data);
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
        exec(data);
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

        exec(data);
        CHECK(psr().n() == get_bit(1490352945, 31));
        CHECK(psr().z() == get_bit(1490352945, 30));
        CHECK(psr().c() == get_bit(1490352945, 29));
        CHECK(psr().v() == get_bit(1490352945, 28));

        // with SPSR and immediate operand
        psr_transfer->operand = 99333394;
        psr_transfer->imm     = true;
        psr_transfer->spsr    = true;
        exec(data);
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
        exec(data);
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
        exec(data);
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
        exec(data);

        // ~54924809
        CHECK(getr(15) == static_cast<uint32_t>(-54924810));

        // flags are not set
        flags(false, false, false, false);
        CHECK(psr(true).raw() == psr().raw());
    }
}

#undef TAG
