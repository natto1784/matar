#include "cpu/cpu-impl.hh"
#include "util/bits.hh"
#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <variant>

using namespace matar;

class CpuFixture {
  public:
    CpuFixture()
      : cpu(Bus(Memory(std::array<uint8_t, Memory::BIOS_SIZE>(),
                       std::vector<uint8_t>(Header::HEADER_SIZE)))) {}

  protected:
    // TODO: test with other conditions
    void exec(arm::InstructionData data, Condition condition = Condition::AL) {
        arm::Instruction instruction(condition, data);
        cpu.exec_arm(instruction);
    }

    void reset(uint32_t value = 0) {
        cpu.pc = value + arm::INSTRUCTION_SIZE * 2;
    }

    CpuImpl cpu;

  private:
    class Null : public std::streambuf {
      public:
        int overflow(int c) override { return c; }
    };
};

static constexpr auto TAG = "[arm][execution]";

using namespace arm;

TEST_CASE_METHOD(CpuFixture, "Branch and Exchange", TAG) {
    InstructionData data = BranchAndExchange{ .rn = 3 };

    cpu.gpr[3] = 342890;

    exec(data);

    CHECK(cpu.pc == 342890);
}

TEST_CASE_METHOD(CpuFixture, "Branch", TAG) {
    InstructionData data = Branch{ .link = false, .offset = 3489748 };
    Branch* branch       = std::get_if<Branch>(&data);

    exec(data);

    CHECK(cpu.pc == 3489748);
    CHECK(cpu.gpr[14] == 0);

    // with link
    reset();
    branch->link = true;
    exec(data);

    CHECK(cpu.pc == 3489748);
    CHECK(cpu.gpr[14] == 0 + INSTRUCTION_SIZE);
}

TEST_CASE_METHOD(CpuFixture, "Multiply", TAG) {
    InstructionData data = Multiply{
        .rm = 10, .rs = 11, .rn = 3, .rd = 5, .set = false, .acc = false
    };
    Multiply* multiply = std::get_if<Multiply>(&data);

    cpu.gpr[10] = 234912349;
    cpu.gpr[11] = 124897;
    cpu.gpr[3]  = 99999;

    {
        uint32_t result = 234912349ull * 124897ull & 0xFFFFFFFF;
        exec(data);

        CHECK(cpu.gpr[5] == result);
    }

    // with accumulate
    {
        uint32_t result = (234912349ull * 124897ull + 99999ull) & 0xFFFFFFFF;
        multiply->acc   = true;
        exec(data);

        CHECK(cpu.gpr[5] == result);
    }

    // with set
    {
        uint32_t result = (234912349ull * 124897ull + 99999ull) & 0xFFFFFFFF;
        multiply->set   = true;
        exec(data);

        CHECK(cpu.gpr[5] == result);
        CHECK(cpu.cpsr.n() == get_bit(result, 31));
    }

    // with set and zero
    {
        cpu.gpr[10] = 0;
        cpu.gpr[3]  = 0;
        exec(data);

        CHECK(cpu.gpr[5] == 0);
        CHECK(cpu.cpsr.n() == false);
        CHECK(cpu.cpsr.z() == true);
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

    cpu.gpr[10] = 234912349;
    cpu.gpr[11] = 124897;

    // unsigned
    {
        uint64_t result = 234912349ull * 124897ull;
        exec(data);

        CHECK(cpu.gpr[3] == bit_range(result, 0, 31));
        CHECK(cpu.gpr[5] == bit_range(result, 32, 63));
    }

    // signed
    {
        int64_t result = 234912349ll * -124897ll;
        cpu.gpr[11] *= -1;
        multiply_long->uns = false;
        exec(data);

        CHECK(cpu.gpr[3] == static_cast<uint32_t>(bit_range(result, 0, 31)));
        CHECK(cpu.gpr[5] == static_cast<uint32_t>(bit_range(result, 32, 63)));
    }

    // accumulate
    {
        cpu.gpr[3] = 99999;
        cpu.gpr[5] = -444333391;

        int64_t result =
          234912349ll * -124897ll + (99999ll | -444333391ll << 32);

        multiply_long->acc = true;
        exec(data);

        CHECK(cpu.gpr[3] == static_cast<uint32_t>(bit_range(result, 0, 31)));
        CHECK(cpu.gpr[5] == static_cast<uint32_t>(bit_range(result, 32, 63)));
    }

    // set
    {
        cpu.gpr[3] = 99999;
        cpu.gpr[5] = -444333391;

        int64_t result =
          234912349ll * -124897ll + (99999ll | -444333391ll << 32);

        multiply_long->set = true;
        exec(data);

        CHECK(cpu.gpr[3] == static_cast<uint32_t>(bit_range(result, 0, 31)));
        CHECK(cpu.gpr[5] == static_cast<uint32_t>(bit_range(result, 32, 63)));
        CHECK(cpu.cpsr.n() == true);
        CHECK(cpu.cpsr.z() == false);
    }

    // zero
    {
        cpu.gpr[10] = 0;
        cpu.gpr[5]  = 0;
        cpu.gpr[3]  = 0;

        exec(data);

        CHECK(cpu.gpr[3] == 0);
        CHECK(cpu.gpr[5] == 0);
        CHECK(cpu.cpsr.n() == false);
        CHECK(cpu.cpsr.z() == true);
    }
}

TEST_CASE_METHOD(CpuFixture, "Single Data Swap", TAG) {
    InstructionData data =
      SingleDataSwap{ .rm = 3, .rd = 4, .rn = 9, .byte = false };
    SingleDataSwap* swap = std::get_if<SingleDataSwap>(&data);

    cpu.gpr[9] = 0x3FED;
    cpu.gpr[3] = 94235087;
    cpu.gpr[3] = -259039045;
    cpu.bus->write_word(cpu.gpr[9], 3241011111);

    SECTION("word") {
        exec(data);

        CHECK(cpu.gpr[4] == 3241011111);
        CHECK(cpu.bus->read_word(cpu.gpr[9]) ==
              static_cast<uint32_t>(-259039045));
    }

    SECTION("byte") {
        swap->byte = true;
        exec(data);

        CHECK(cpu.gpr[4] == (3241011111 & 0xFF));
        CHECK(cpu.bus->read_byte(cpu.gpr[9]) ==
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

    cpu.gpr[3] = 1596;
    cpu.gpr[7] = 6;
    cpu.gpr[5] = -911111;

    // shifted register (immediate)
    {
        // 12768 + 6
        cpu.bus->write_word(12774, 95995);
        exec(data);

        CHECK(cpu.gpr[5] == 95995);
        cpu.gpr[5] = 0;
    }

    // shifted register (register)
    {
        data_transfer->offset = Shift{ .rm   = 3,
                                       .data = ShiftData{
                                         .type      = ShiftType::LSL,
                                         .immediate = false,
                                         .operand   = 12,
                                       } };

        cpu.gpr[12] = 2;
        // 6384 + 6
        cpu.bus->write_word(6390, 3948123487);
        exec(data);

        CHECK(cpu.gpr[5] == 3948123487);
    }

    // immediate
    {
        data_transfer->offset = static_cast<uint16_t>(3489);
        // 6 + 3489
        cpu.bus->write_word(3495, 68795467);

        exec(data);

        CHECK(cpu.gpr[5] == 68795467);
    }

    // down
    {
        cpu.gpr[7]        = 18044;
        data_transfer->up = false;
        // 18044 - 3489
        cpu.bus->write_word(14555, 5949595);

        exec(data);

        CHECK(cpu.gpr[5] == 5949595);
        // no write back
        CHECK(cpu.gpr[7] == 18044);
    }

    // write
    {
        data_transfer->write = true;
        cpu.bus->write_word(14555, 967844);

        exec(data);

        CHECK(cpu.gpr[5] == 967844);
        // 18044 - 3489
        CHECK(cpu.gpr[7] == 14555);
    }

    // post
    {
        data_transfer->write = false;
        data_transfer->pre   = false;
        cpu.bus->write_word(14555, 61119);

        exec(data);

        CHECK(cpu.gpr[5] == 61119);
        // 14555 - 3489
        CHECK(cpu.gpr[7] == 11066);
    }

    // store
    {
        data_transfer->load = false;

        exec(data);

        CHECK(cpu.bus->read_word(11066) == 61119);
        // 11066 - 3489
        CHECK(cpu.gpr[7] == 7577);
    }

    // r15 as rn
    {
        data_transfer->rn = 15;
        cpu.gpr[15]       = 7577;

        exec(data);

        CHECK(cpu.bus->read_word(7577 - 2 * INSTRUCTION_SIZE) == 61119);
        // 7577 - 3489
        CHECK(cpu.gpr[15] == 4088 - 2 * INSTRUCTION_SIZE);

        // cleanup
        data_transfer->rn = 7;
    }

    // r15 as rd
    {
        // 4088
        data_transfer->rd = 15;
        cpu.gpr[15]       = 444444;

        exec(data);

        CHECK(cpu.bus->read_word(7577 + INSTRUCTION_SIZE) == 444444);
        // 7577 - 3489
        CHECK(cpu.gpr[7] == 4088 + INSTRUCTION_SIZE);

        // cleanup
        data_transfer->rd = 5;
        cpu.gpr[7] -= INSTRUCTION_SIZE;
    }

    // byte
    {
        data_transfer->byte = true;

        cpu.gpr[5] = 458267584;

        exec(data);

        CHECK(cpu.bus->read_word(4088) == (458267584 & 0xFF));
        // 4088 - 3489
        CHECK(cpu.gpr[7] == 599);
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

    cpu.gpr[12] = 8404;
    cpu.gpr[11] = 459058287;
    cpu.gpr[10] = 900;

    // register offset
    {
        // 900 + 8404
        cpu.bus->write_word(9304, 3948123487);
        exec(data);

        CHECK(cpu.gpr[11] == (3948123487 & 0xFFFF));
    }

    // immediate offset
    {
        hw_transfer->imm    = true;
        hw_transfer->offset = 167;
        // 900 + 167
        cpu.bus->write_word(1067, 594633302);
        exec(data);

        CHECK(cpu.gpr[11] == (594633302 & 0xFFFF));
    }

    // down
    {
        hw_transfer->up = false;
        // 900 - 167
        cpu.bus->write_word(733, 222221);

        exec(data);

        CHECK(cpu.gpr[11] == (222221 & 0xFFFF));
        // no write back
        CHECK(cpu.gpr[10] == 900);
    }

    // write
    {
        hw_transfer->write = true;
        // 900 - 167
        cpu.bus->write_word(733, 100000005);

        exec(data);

        CHECK(cpu.gpr[11] == (100000005 & 0xFFFF));
        // 900 - 167
        CHECK(cpu.gpr[10] == 733);
    }

    // post
    {
        hw_transfer->pre   = false;
        hw_transfer->write = false;
        cpu.bus->write_word(733, 6111909);

        exec(data);

        CHECK(cpu.gpr[11] == (6111909 & 0xFFFF));
        // 733 - 167
        CHECK(cpu.gpr[10] == 566);
    }

    // store
    {
        hw_transfer->load = false;

        exec(data);

        CHECK(cpu.bus->read_halfword(566) == (6111909 & 0xFFFF));
        // 566 - 167
        CHECK(cpu.gpr[10] == 399);
    }

    // r15 as rn
    {
        hw_transfer->rn = 15;
        cpu.gpr[15]     = 399;

        exec(data);

        CHECK(cpu.bus->read_halfword(399 - 2 * INSTRUCTION_SIZE) ==
              (6111909 & 0xFFFF));
        // 399 - 167
        CHECK(cpu.gpr[15] == 232 - 2 * INSTRUCTION_SIZE);

        // cleanup
        hw_transfer->rn = 10;
    }

    // r15 as rd
    {
        hw_transfer->rd = 15;
        cpu.gpr[15]     = 224;

        exec(data);

        CHECK(cpu.bus->read_halfword(399 + INSTRUCTION_SIZE) == 224);
        // 399 - 167
        CHECK(cpu.gpr[10] == 232 + INSTRUCTION_SIZE);

        // cleanup
        hw_transfer->rd = 11;
        cpu.gpr[10]     = 399;
    }

    // signed halfword
    {
        hw_transfer->load = true;
        hw_transfer->sign = true;
        cpu.bus->write_halfword(399, -12345);

        exec(data);

        CHECK(cpu.gpr[11] == static_cast<uint32_t>(-12345));
        // 399 - 167
        CHECK(cpu.gpr[10] == 232);
    }

    // signed byte
    {
        hw_transfer->half = false;
        cpu.bus->write_byte(232, -56);

        exec(data);

        CHECK(cpu.gpr[11] == static_cast<uint32_t>(-56));
        // 232 - 167
        CHECK(cpu.gpr[10] == 65);
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
        cpu.bus->write_word(3448, 38947234);
        cpu.bus->write_word(3452, 237164);
        cpu.bus->write_word(3456, 679785111);
        cpu.bus->write_word(3460, 905895898);
        cpu.bus->write_word(3464, 131313333);
        cpu.bus->write_word(3468, 131);
        cpu.bus->write_word(3472, 989231);
        cpu.bus->write_word(3476, 6);

        auto checker = [](decltype(cpu.gpr)& gpr, uint32_t rnval = 0) {
            CHECK(gpr[0] == 237164);
            CHECK(gpr[1] == 0);
            CHECK(gpr[2] == 0);
            CHECK(gpr[3] == 0);
            CHECK(gpr[4] == 0);
            CHECK(gpr[5] == 0);
            CHECK(gpr[6] == 679785111);
            CHECK(gpr[7] == 905895898);
            CHECK(gpr[8] == 131313333);
            CHECK(gpr[9] == 0);
            CHECK(gpr[10] == rnval);
            CHECK(gpr[11] == 131);
            CHECK(gpr[12] == 0);
            CHECK(gpr[13] == 989231);
            CHECK(gpr[14] == 0);
            CHECK(gpr[15] == 6);

            for (uint8_t i = 0; i < 16; i++) {
                gpr[i] = 0;
            }
        };

        cpu.gpr[10] = 3448;
        exec(data);
        checker(cpu.gpr, 3448);

        // with write
        cpu.gpr[10]           = 3448;
        block_transfer->write = true;
        exec(data);
        checker(cpu.gpr, 3448 + INSTRUCTION_SIZE);

        // decrement
        block_transfer->write = false;
        block_transfer->up    = false;
        // adjust rn
        cpu.gpr[10] = 3480;
        exec(data);
        checker(cpu.gpr, 3480);

        // with write
        cpu.gpr[10]           = 3480;
        block_transfer->write = true;
        exec(data);
        checker(cpu.gpr, 3480 - INSTRUCTION_SIZE);

        // post increment
        block_transfer->write = false;
        block_transfer->up    = true;
        block_transfer->pre   = false;
        // adjust rn
        cpu.gpr[10] = 3452;
        exec(data);
        checker(cpu.gpr, 3452 + INSTRUCTION_SIZE);

        // post decrement
        block_transfer->up = false;
        // adjust rn
        cpu.gpr[10] = 3476;
        exec(data);
        checker(cpu.gpr, 3476 - INSTRUCTION_SIZE);

        // with s bit
        cpu.chg_mode(Mode::Fiq);
        block_transfer->s = true;
        CHECK(cpu.cpsr.raw() != cpu.spsr.raw());
        exec(data);
        CHECK(cpu.cpsr.raw() == cpu.spsr.raw());
    }

    // store
    SECTION("store") {
        block_transfer->load = false;

        // populate registers
        cpu.gpr[0]  = 237164;
        cpu.gpr[6]  = 679785111;
        cpu.gpr[7]  = 905895898;
        cpu.gpr[8]  = 131313333;
        cpu.gpr[11] = 131;
        cpu.gpr[13] = 989231;
        cpu.gpr[15] = 6;

        auto checker = [this]() {
            CHECK(cpu.bus->read_word(5548) == 237164);
            CHECK(cpu.bus->read_word(5552) == 679785111);
            CHECK(cpu.bus->read_word(5556) == 905895898);
            CHECK(cpu.bus->read_word(5560) == 131313333);
            CHECK(cpu.bus->read_word(5564) == 131);
            CHECK(cpu.bus->read_word(5568) == 989231);
            CHECK(cpu.bus->read_word(5572) == 6);

            for (uint8_t i = 0; i < 8; i++)
                cpu.bus->write_word(5548 + i * 4, 0);
        };

        cpu.gpr[10] = 5544; // base
        exec(data);
        checker();

        // decrement
        block_transfer->write = false;
        block_transfer->up    = false;
        // adjust rn
        cpu.gpr[10] = 5576;
        exec(data);
        checker();

        // post increment
        block_transfer->up  = true;
        block_transfer->pre = false;
        // adjust rn
        cpu.gpr[10] = 5548;
        exec(data);
        checker();

        // post decrement
        block_transfer->up = false;
        // adjust rn
        cpu.gpr[10] = 5572;
        exec(data);
        checker();

        // with s bit
        cpu.chg_mode(Mode::Fiq);
        block_transfer->s = true;
        cpu.chg_mode(Mode::Supervisor);
        // User's R13 is different (unset at this point)
        CHECK(cpu.bus->read_word(5568) == 0);
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
        cpu.gpr[12] = 12389398;
        CHECK(cpu.cpsr.raw() != cpu.gpr[12]);
        exec(data);
        CHECK(cpu.cpsr.raw() == cpu.gpr[12]);

        psr_transfer->spsr = true;
        // with SPSR
        CHECK(cpu.spsr.raw() != cpu.gpr[12]);
        exec(data);
        CHECK(cpu.spsr.raw() == cpu.gpr[12]);
    }

    // MSR
    SECTION("MSR") {
        psr_transfer->type = PsrTransfer::Type::Msr;

        cpu.gpr[12] = 0;
        // go to the reserved bits
        cpu.gpr[12] |= 16556 << 8;

        CHECK(cpu.cpsr.raw() != cpu.gpr[12]);
        exec(data);
        CHECK(cpu.cpsr.raw() == cpu.gpr[12]);

        psr_transfer->spsr = true;
        // with SPSR
        CHECK(cpu.spsr.raw() != cpu.gpr[12]);
        exec(data);
        CHECK(cpu.spsr.raw() == cpu.gpr[12]);
    }

    // MSR_flg
    SECTION("MSR_flg") {
        psr_transfer->type = PsrTransfer::Type::Msr_flg;

        cpu.gpr[12] = 1490352945;
        // go to the reserved bits

        exec(data);
        CHECK(cpu.cpsr.n() == get_bit(1490352945, 31));
        CHECK(cpu.cpsr.z() == get_bit(1490352945, 30));
        CHECK(cpu.cpsr.c() == get_bit(1490352945, 29));
        CHECK(cpu.cpsr.v() == get_bit(1490352945, 28));

        // with SPSR and immediate operand
        psr_transfer->operand = 99333394;
        psr_transfer->imm     = true;
        psr_transfer->spsr    = true;
        exec(data);
        CHECK(cpu.spsr.n() == get_bit(9933394, 31));
        CHECK(cpu.spsr.z() == get_bit(9933394, 30));
        CHECK(cpu.spsr.c() == get_bit(9933394, 29));
        CHECK(cpu.spsr.v() == get_bit(9933394, 28));
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
    cpu.gpr[7] = -28717;

    // AND with shifted register (imediate)
    {
        // rm
        cpu.gpr[3] = 1596;
        exec(data);
        // -28717 & 12768
        CHECK(cpu.gpr[5] == 448);
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
        cpu.gpr[3] = 1596;
        // rs
        cpu.gpr[12] = 2;
        exec(data);
        // -28717 & 6384
        CHECK(cpu.gpr[5] == 2256);
    }

    // same as above but with rn (oprerand 1) = 15
    {
        processing->rn = 15;
        cpu.gpr[15]    = -2871;
        exec(data);

        // (-2871 + INSTRUCTION_SIZE) & 6384
        CHECK(cpu.gpr[5] == ((-2871 + INSTRUCTION_SIZE) & 6384));

        // cleanup
        processing->rn = 7;
    }

    auto flags = [this](bool n, bool z, bool v, bool c) {
        CHECK(cpu.cpsr.n() == n);
        CHECK(cpu.cpsr.z() == z);
        CHECK(cpu.cpsr.v() == v);
        CHECK(cpu.cpsr.c() == c);

        cpu.cpsr.set_n(false);
        cpu.cpsr.set_z(false);
        cpu.cpsr.set_v(false);
        cpu.cpsr.set_c(false);
    };

    // immediate operand
    processing->operand = static_cast<uint32_t>(54924809);
    // rs
    cpu.gpr[12] = 2;
    cpu.gpr[5]  = 0;

    SECTION("AND") {
        processing->opcode = OpCode::AND;
        exec(data);

        // -28717 & 54924809
        CHECK(cpu.gpr[5] == 54920705);

        // check set flags
        flags(false, false, false, false);
    }

    // TST with immediate operand
    SECTION("TST") {
        processing->opcode = OpCode::TST;

        exec(data);

        // -28717 & 54924809
        CHECK(cpu.gpr[5] == 0);

        // check set flags
        flags(false, false, false, false);
    }

    SECTION("EOR") {
        processing->opcode = OpCode::EOR;
        exec(data);

        // -28717 ^ 54924809
        CHECK(cpu.gpr[5] == 4240021978);

        // check set flags
        flags(true, false, false, false);

        // check zero flag
        processing->operand = static_cast<uint32_t>(-28717);
        exec(data);

        // -28717 ^ -28717
        CHECK(cpu.gpr[5] == 0);

        // check set flags
        flags(false, true, false, false);
    }

    SECTION("TEQ") {
        processing->opcode = OpCode::TEQ;

        exec(data);

        // -28717 ^ 54924809
        CHECK(cpu.gpr[5] == 0);

        // check set flags
        flags(true, false, false, false);
    }

    SECTION("SUB") {
        processing->opcode = OpCode::SUB;
        exec(data);

        // -28717 - 54924809
        CHECK(cpu.gpr[5] == static_cast<uint32_t>(-54953526));

        // check set flags
        flags(true, false, false, true);

        // check zero flag
        processing->operand = static_cast<uint32_t>(-28717);
        exec(data);

        // -28717 - (-28717)
        CHECK(cpu.gpr[5] == 0);

        // check set flags
        flags(false, true, false, true);
    }

    SECTION("CMP") {
        processing->opcode = OpCode::CMP;

        exec(data);

        // -28717 - 54924809
        CHECK(cpu.gpr[5] == 0);

        // check set flags
        flags(true, false, false, true);
    }

    SECTION("RSB") {
        processing->opcode = OpCode::RSB;
        exec(data);

        // +28717 + 54924809
        CHECK(cpu.gpr[5] == 54953526);

        // check set flags
        flags(false, false, false, false);
    }

    SECTION("ADD") {
        processing->opcode = OpCode::ADD;
        exec(data);

        // -28717 + 54924809
        CHECK(cpu.gpr[5] == 54896092);

        // check set flags
        flags(false, false, false, false);

        // test zero flag
        processing->operand = static_cast<uint32_t>(28717);
        exec(data);

        // -28717 + 28717
        CHECK(cpu.gpr[5] == 0);

        // check set flags
        flags(false, true, false, false);

        // test overflow flag
        processing->operand = static_cast<uint32_t>((1u << 31) - 1);
        cpu.gpr[7]          = (1u << 31) - 1;

        exec(data);

        CHECK(cpu.gpr[5] == (1ull << 32) - 2);

        // check set flags
        flags(true, false, true, false);
    }

    SECTION("CMN") {
        processing->opcode = OpCode::CMN;

        exec(data);

        // -28717 + 54924809
        CHECK(cpu.gpr[5] == 0);

        // check set flags
        flags(false, false, false, false);
    }

    SECTION("ADC") {
        processing->opcode = OpCode::ADC;
        cpu.cpsr.set_c(true);
        exec(data);

        // -28717 + 54924809 + carry
        CHECK(cpu.gpr[5] == 54896093);

        // check set flags
        flags(false, false, false, false);
    }

    SECTION("SBC") {
        processing->opcode = OpCode::SBC;
        cpu.cpsr.set_c(false);
        exec(data);

        // -28717 - 54924809 + carry - 1
        CHECK(cpu.gpr[5] == static_cast<uint32_t>(-54953527));

        // check set flags
        flags(true, false, false, false);
    }

    SECTION("RSC") {
        processing->opcode = OpCode::RSC;
        cpu.cpsr.set_c(false);
        exec(data);

        // +28717 +54924809 + carry - 1
        CHECK(cpu.gpr[5] == 54953525);

        // check set flags
        flags(false, false, false, false);
    }

    SECTION("ORR") {
        processing->opcode = OpCode::ORR;
        exec(data);

        // -28717 | 54924809
        CHECK(cpu.gpr[5] == static_cast<uint32_t>(-24613));

        // check set flags
        flags(true, false, false, false);
    }

    SECTION("BIC") {
        processing->opcode = OpCode::BIC;
        exec(data);

        // -28717 & ~54924809
        CHECK(cpu.gpr[5] == static_cast<uint32_t>(-54949422));

        // check set flags
        flags(true, false, false, false);
    }

    SECTION("MVN") {
        processing->opcode = OpCode::MVN;
        exec(data);

        // ~54924809
        CHECK(cpu.gpr[5] == static_cast<uint32_t>(-54924810));

        // check set flags
        flags(true, false, false, false);
    }

    SECTION("R15 as destination") {
        processing->opcode = OpCode::MVN;
        processing->rd     = 15;
        cpu.gpr[15]        = 0;
        CHECK(cpu.spsr.raw() != cpu.cpsr.raw());
        exec(data);

        // ~54924809
        CHECK(cpu.gpr[15] == static_cast<uint32_t>(-54924810));

        // flags are not set
        flags(false, false, false, false);
        CHECK(cpu.spsr.raw() == cpu.cpsr.raw());
    }
}
