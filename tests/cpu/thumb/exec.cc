#include "cpu/cpu-fixture.hh"
#include "cpu/thumb/instruction.hh"
#include <catch2/catch_test_macros.hpp>

using namespace matar;

#define TAG "[thumb][execution]"

using namespace thumb;

TEST_CASE_METHOD(CpuFixture, "Move Shifted Register", TAG) {
    InstructionData data = MoveShiftedRegister{
        .rd = 3, .rs = 5, .offset = 15, .opcode = ShiftType::LSL
    };
    MoveShiftedRegister* move = std::get_if<MoveShiftedRegister>(&data);

    SECTION("LSL") {
        setr(3, 0);
        setr(5, 6687);
        // LSL
        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 1); // 1S
        CHECK(getr(3) == 219119616);

        setr(5, 0);
        // zero
        exec(data);
        CHECK(getr(3) == 0);
        CHECK(psr().z());
    }

    SECTION("LSR") {
        move->opcode = ShiftType::LSR;
        setr(5, -1827489745);
        // LSR

        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 1); // 1S

        CHECK(getr(3) == 75301);
        CHECK(!psr().n());

        setr(5, 4444);
        // zero flag
        exec(data);
        CHECK(getr(3) == 0);
        CHECK(psr().z());
    }

    SECTION("ASR") {
        setr(5, -1827489745);
        move->opcode = ShiftType::ASR;
        // ASR

        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 1); // 1S

        CHECK(psr().n());
        CHECK(getr(3) == 4294911525);

        setr(5, 500);
        // zero flag
        exec(data);
        CHECK(getr(3) == 0);
        CHECK(psr().z());
    }
}

TEST_CASE_METHOD(CpuFixture, "Add/Subtract", TAG) {
    InstructionData data = AddSubtract{ .rd     = 5,
                                        .rs     = 2,
                                        .offset = 7,
                                        .opcode = AddSubtract::OpCode::ADD,
                                        .imm    = false };
    AddSubtract* add     = std::get_if<AddSubtract>(&data);
    setr(2, 378427891);
    setr(7, -666666);

    SECTION("ADD") {
        // register
        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 1); // 1S

        CHECK(getr(5) == 377761225);

        add->imm = true;
        setr(2, (1u << 31) - 1);
        // immediate and overflow
        exec(data);
        CHECK(getr(5) == 2147483654);
        CHECK(psr().v());

        setr(2, -7);
        // zero
        exec(data);
        CHECK(getr(5) == 0);
        CHECK(psr().z());
    }

    add->imm = true;

    SECTION("SUB") {
        add->opcode = AddSubtract::OpCode::SUB;
        setr(2, -((1u << 31) - 1));
        add->offset = 4;

        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 1); // 1S

        CHECK(getr(5) == 2147483645);
        CHECK(psr().v());

        setr(2, ~0u);
        add->offset = -4;
        // carry
        exec(data);
        CHECK(getr(5) == 3);
        CHECK(psr().c());

        setr(2, 0);
        add->offset = 0;
        // zero
        exec(data);

        CHECK(getr(5) == 0);
        CHECK(psr().z());
    }
}

TEST_CASE_METHOD(CpuFixture, "Move/Compare/Add/Subtract Immediate", TAG) {
    InstructionData data = MovCmpAddSubImmediate{
        .offset = 251, .rd = 5, .opcode = MovCmpAddSubImmediate::OpCode::MOV
    };
    MovCmpAddSubImmediate* move = std::get_if<MovCmpAddSubImmediate>(&data);

    SECTION("MOV") {
        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 1); // 1S

        CHECK(getr(5) == 251);

        move->offset = 0;
        // zero
        exec(data);
        CHECK(getr(5) == 0);
        CHECK(psr().z());
    }

    SECTION("CMP") {
        setr(5, 251);
        move->opcode = MovCmpAddSubImmediate::OpCode::CMP;
        CHECK(!psr().z());

        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 1); // 1S

        CHECK(getr(5) == 251);
        CHECK(psr().z());

        // overflow
        setr(5, -((1u << 31) - 1));
        CHECK(!psr().v());
        exec(data);
        CHECK(getr(5) == 2147483649);
        CHECK(psr().v());
    }

    SECTION("ADD") {
        move->opcode = MovCmpAddSubImmediate::OpCode::ADD;
        setr(5, (1u << 31) - 1);
        // immediate and overflow

        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 1); // 1S

        CHECK(getr(5) == 2147483898);
        CHECK(psr().v());

        setr(5, -251);
        // zero
        exec(data);
        CHECK(getr(5) == 0);
        CHECK(psr().z());
    }

    SECTION("SUB") {
        // same as CMP but loaded
        setr(5, 251);
        move->opcode = MovCmpAddSubImmediate::OpCode::SUB;
        CHECK(!psr().z());

        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 1); // 1S

        CHECK(getr(5) == 0);
        CHECK(psr().z());

        // overflow
        setr(5, -((1u << 31) - 1));
        CHECK(!psr().v());
        exec(data);
        CHECK(getr(5) == 2147483398);
        CHECK(psr().v());
    }
}

TEST_CASE_METHOD(CpuFixture, "ALU Operations", TAG) {
    InstructionData data =
      AluOperations{ .rd = 1, .rs = 3, .opcode = AluOperations::OpCode::AND };
    AluOperations* alu = std::get_if<AluOperations>(&data);

    setr(1, 328940001);
    setr(3, -991);

    SECTION("AND") {
        uint32_t cycles = bus->get_cycles();
        // 328940001 & -991
        exec(data);
        CHECK(bus->get_cycles() == cycles + 1); // 1S

        CHECK(getr(1) == 328939553);
        CHECK(!psr().n());

        setr(3, 0);
        CHECK(!psr().z());
        // zero
        exec(data);
        CHECK(getr(1) == 0);
        CHECK(psr().z());
    }

    SECTION("EOR") {
        alu->opcode = AluOperations::OpCode::EOR;
        // 328940001 ^ -991
        exec(data);
        CHECK(getr(1) == 3966027200);
        CHECK(psr().n());

        setr(3, 3966027200);
        // zero
        exec(data);
        CHECK(getr(1) == 0);
        CHECK(psr().z());
        CHECK(!psr().n());
    }

    SECTION("LSL") {
        setr(3, 3);
        alu->opcode = AluOperations::OpCode::LSL;

        uint32_t cycles = bus->get_cycles();
        // 328940001 << 3
        exec(data);
        CHECK(bus->get_cycles() == cycles + 2); // 1S + 1I (shift)

        CHECK(getr(1) == 2631520008);
        CHECK(psr().n());

        setr(1, 0);
        // zero
        exec(data);
        CHECK(getr(1) == 0);
        CHECK(psr().z());
    }

    SECTION("LSR") {
        alu->opcode = AluOperations::OpCode::LSR;
        setr(3, 991);
        // 328940001 >> 991
        exec(data);
        CHECK(getr(1) == 0);
        CHECK(psr().z());

        setr(1, -83885328);
        setr(3, 5);
        // -83885328 >> 5
        exec(data);
        CHECK(getr(1) == 131596311);
        CHECK(!psr().z());
        CHECK(!psr().n());
    }

    SECTION("ASR") {
        alu->opcode = AluOperations::OpCode::ASR;
        setr(3, 991);
        // 328940001 >> 991
        exec(data);
        CHECK(getr(1) == 0);
        CHECK(psr().z());

        setr(1, -83885328);
        setr(3, 5);
        // -83885328 >> 5
        exec(data);
        CHECK(getr(1) == 4292345879);
        CHECK(!psr().z());
        CHECK(psr().n());
    }

    SECTION("ADC") {
        alu->opcode = AluOperations::OpCode::ADC;
        setr(3, (1u << 31) - 1);
        Psr cpsr = psr();
        cpsr.set_c(true);
        set_psr(cpsr);
        // 2147483647 + 328940001 + 1
        exec(data);
        CHECK(getr(1) == 2476423649);
        CHECK(psr().v());
        CHECK(psr().n());
        CHECK(!psr().c());

        setr(3, -328940001);
        setr(1, 328940001);
        // zero
        exec(data);
        CHECK(getr(1) == 0);
        CHECK(psr().z());
    }

    SECTION("SBC") {
        alu->opcode = AluOperations::OpCode::SBC;
        setr(3, -((1u << 31) - 1));

        Psr cpsr = psr();
        cpsr.set_c(false);
        set_psr(cpsr);

        // 328940001 - -2147483647 - 1
        exec(data);
        CHECK(getr(1) == 2476423647);
        CHECK(psr().v());
        CHECK(psr().n());
        CHECK(!psr().c());

        setr(1, -34892);
        setr(3, -34893);
        // zero
        exec(data);
        CHECK(getr(1) == 0);
        CHECK(psr().z());
    }

    SECTION("ROR") {
        setr(3, 993);
        alu->opcode = AluOperations::OpCode::ROR;
        // 328940001 ROR 993
        exec(data);
        CHECK(getr(1) == 2311953648);
        CHECK(psr().n());
        CHECK(psr().c());

        setr(1, 0);
        // zero
        exec(data);
        CHECK(getr(1) == 0);
        CHECK(psr().z());
    }

    SECTION("TST") {
        alu->opcode = AluOperations::OpCode::TST;
        // 328940001 & -991
        exec(data);
        // no change
        CHECK(getr(1) == 328940001);

        setr(3, 0);
        CHECK(!psr().z());
        // zero
        exec(data);
        CHECK(getr(1) == 328940001);
        CHECK(psr().z());
    }

    SECTION("NEG") {
        alu->opcode = AluOperations::OpCode::NEG;
        // -(-991)
        exec(data);
        CHECK(getr(1) == 991);

        setr(3, 0);
        // zero
        exec(data);
        CHECK(getr(1) == 0);
        CHECK(psr().z());
    }

    SECTION("CMP") {
        alu->opcode = AluOperations::OpCode::CMP;
        setr(3, -((1u << 31) - 1));
        // 328940001 - -2147483647
        exec(data);
        // no change
        CHECK(getr(1) == 328940001);
        CHECK(psr().v());
        CHECK(psr().n());
        CHECK(!psr().c());

        setr(1, -34892);
        setr(3, -34892);
        // zero
        exec(data);
        // no change (-34892)
        CHECK(getr(1) == 4294932404);
        CHECK(psr().z());
    }

    SECTION("CMN") {
        alu->opcode = AluOperations::OpCode::CMN;
        setr(3, (1u << 31) - 1);
        // 2147483647 + 328940001
        exec(data);
        CHECK(getr(1) == 328940001);
        CHECK(psr().v());
        CHECK(psr().n());
        CHECK(!psr().c());

        setr(3, -328940001);
        setr(1, 328940001);
        // zero
        exec(data);
        CHECK(getr(1) == 328940001);
        CHECK(psr().z());
    }

    SECTION("ORR") {
        alu->opcode = AluOperations::OpCode::ORR;
        // 328940001 | -991
        exec(data);
        CHECK(getr(1) == 4294966753);
        CHECK(psr().n());

        setr(1, 0);
        setr(3, 0);
        // zero
        exec(data);
        CHECK(getr(1) == 0);
        CHECK(psr().z());
    }

    SECTION("MUL") {
        alu->opcode = AluOperations::OpCode::MUL;

        uint32_t cycles = bus->get_cycles();
        // 328940001 * -991 (lower 32 bits) (-325979540991 & 0xFFFFFFFF)
        exec(data);
        CHECK(bus->get_cycles() == cycles + 3); // S + mI (m = 2 for -991)

        CHECK(getr(1) == 437973505);

        setr(3, 0);
        // zero
        exec(data);
        CHECK(getr(1) == 0);
        CHECK(psr().z());
    }

    SECTION("BIC") {
        alu->opcode = AluOperations::OpCode::BIC;
        // 328940001 & ~ -991
        exec(data);
        CHECK(getr(1) == 448);
        CHECK(!psr().n());

        setr(3, ~0u);
        // zero
        exec(data);
        CHECK(getr(1) == 0);
        CHECK(psr().z());
    }

    SECTION("MVN") {
        alu->opcode = AluOperations::OpCode::MVN;
        //~ -991
        exec(data);
        CHECK(getr(1) == 990);
        CHECK(!psr().n());

        setr(3, 24358);
        // negative
        exec(data);
        CHECK(getr(1) == 4294942937);
        CHECK(psr().n());

        setr(3, ~0u);
        // zero
        exec(data);
        CHECK(getr(1) == 0);
        CHECK(psr().z());
    }
}

TEST_CASE_METHOD(CpuFixture, "Hi Register Operations/Branch Exchange", TAG) {
    InstructionData data = HiRegisterOperations{
        .rd = 5, .rs = 15, .opcode = HiRegisterOperations::OpCode::ADD
    };
    HiRegisterOperations* hi = std::get_if<HiRegisterOperations>(&data);

    setr(15, 3452948948);
    setr(5, 958656720);

    SECTION("ADD") {
        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 1); // 1S

        CHECK(getr(5) == 116638372);

        // hi + hi
        hi->rd = 14;
        hi->rs = 15;
        setr(14, 42589);
        exec(data);
        CHECK(getr(14) == 3452991537);
    }

    SECTION("CMP") {
        hi->opcode = HiRegisterOperations::OpCode::CMP;
        exec(data);

        // no change
        CHECK(getr(5) == 958656720);
        CHECK(!psr().n());
        CHECK(!psr().c());
        CHECK(!psr().v());
        CHECK(!psr().z());

        setr(15, 958656720);
        // zero
        exec(data);
        // no change
        CHECK(getr(5) == 958656720);
        CHECK(psr().z());
    }

    SECTION("MOV") {
        hi->opcode = HiRegisterOperations::OpCode::MOV;
        exec(data);

        CHECK(getr(5) == 3452948948);
    }

    SECTION("BX") {
        hi->opcode = HiRegisterOperations::OpCode::BX;
        hi->rs     = 10;

        SECTION("Arm") {
            setr(10, 2189988);

            uint32_t cycles = bus->get_cycles();
            exec(data);
            CHECK(bus->get_cycles() == cycles + 3); // 2S + N cycles

            // +4 for pipeline flush
            CHECK(getr(15) == 2189988 + 4);
            // switched to arm
            CHECK(psr().state() == State::Arm);
        }

        SECTION("Thumb") {
            setr(10, 2189989);
            exec(data);

            // +4 for pipeline flush
            CHECK(getr(15) == 2189988 + 4);

            // switched to thumb
            CHECK(psr().state() == State::Thumb);
        }
    }
}

TEST_CASE_METHOD(CpuFixture, "PC Relative Load", TAG) {
    InstructionData data = PcRelativeLoad{ .word = 0x578, .rd = 0 };

    setr(15, 0x3003FD5);
    // resetting bit 0 for 0x3003FD5, we get 0x3003FD4
    // 0x3003FD4 + 0x578
    bus->write_word(0x300454C, 489753492);

    CHECK(getr(0) == 0);

    uint32_t cycles = bus->get_cycles();
    exec(data);
    CHECK(bus->get_cycles() == cycles + 3); // S + N + I cycles

    CHECK(getr(0) == 489753492);
}

TEST_CASE_METHOD(CpuFixture, "Load/Store with Register Offset", TAG) {
    InstructionData data = LoadStoreRegisterOffset{
        .rd = 3, .rb = 0, .ro = 7, .byte = false, .load = false
    };
    LoadStoreRegisterOffset* load = std::get_if<LoadStoreRegisterOffset>(&data);

    setr(7, 0x3003000);
    setr(0, 0x332);
    setr(3, 389524259);

    SECTION("store") {
        // 0x3003000 + 0x332
        CHECK(bus->read_word(0x3003332) == 0);

        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 2); // 2N cycles

        CHECK(bus->read_word(0x3003332) == 389524259);

        // byte
        load->byte = true;
        bus->write_word(0x3003332, 0);
        exec(data);
        CHECK(bus->read_word(0x3003332) == 35);
    }

    SECTION("load") {
        load->load = true;
        bus->write_word(0x3003332, 11123489);

        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 3); // S + N + I cycles

        CHECK(getr(3) == 11123489);

        // byte
        load->byte = true;
        exec(data);
        CHECK(getr(3) == 33);
    }
}

TEST_CASE_METHOD(CpuFixture, "Load/Store Sign Extended Byte/Halfword", TAG) {
    InstructionData data = LoadStoreSignExtendedHalfword{
        .rd = 3, .rb = 0, .ro = 7, .s = false, .h = false
    };
    LoadStoreSignExtendedHalfword* load =
      std::get_if<LoadStoreSignExtendedHalfword>(&data);

    setr(7, 0x3003000);
    setr(0, 0x332);
    setr(3, 389524259);

    SECTION("SH = 00") {
        // 0x3003000 + 0x332
        CHECK(bus->read_word(0x3003332) == 0);
        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 2); // 2N cycles
        CHECK(bus->read_word(0x3003332) == 43811);
    }

    SECTION("SH = 01") {
        load->h = true;
        bus->write_word(0x3003332, 11123489);
        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 3); // S + N + I cycles
        CHECK(getr(3) == 47905);
    }

    SECTION("SH = 10") {
        load->s = true;
        bus->write_word(0x3003332, 34521594);
        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 3); // S + N + I cycles
        // sign extended 250 byte (0xFA)
        CHECK(getr(3) == 4294967290);
    }

    SECTION("SH = 11") {
        load->s = true;
        load->h = true;
        bus->write_word(0x3003332, 11123489);
        // sign extended 47905 halfword (0xBB21)
        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 3); // S + N + I cycles
        CHECK(getr(3) == 4294949665);
    }
}

TEST_CASE_METHOD(CpuFixture, "Load/Store with Immediate Offset", TAG) {
    InstructionData data = LoadStoreImmediateOffset{
        .rd = 3, .rb = 0, .offset = 0x6E, .load = false, .byte = false
    };
    LoadStoreImmediateOffset* load =
      std::get_if<LoadStoreImmediateOffset>(&data);

    setr(0, 0x300666A);
    setr(3, 389524259);

    SECTION("store") {
        // 0x30066A + 0x6E
        CHECK(bus->read_word(0x30066D8) == 0);
        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 2); // 2N cycles
        CHECK(bus->read_word(0x30066D8) == 389524259);

        // byte
        load->byte = true;
        bus->write_word(0x30066D8, 0);
        exec(data);
        CHECK(bus->read_word(0x30066D8) == 35);
    }

    SECTION("load") {
        load->load = true;
        bus->write_word(0x30066D8, 11123489);
        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 3); // S + N + I cycles
        CHECK(getr(3) == 11123489);

        // byte
        load->byte = true;
        exec(data);
        CHECK(getr(3) == 33);
    }
}

TEST_CASE_METHOD(CpuFixture, "Load/Store Halfword", TAG) {
    InstructionData data =
      LoadStoreHalfword{ .rd = 3, .rb = 0, .offset = 0x6E, .load = false };
    LoadStoreHalfword* load = std::get_if<LoadStoreHalfword>(&data);

    setr(0, 0x300666A);
    setr(3, 389524259);

    SECTION("store") {
        // 0x300666A + 0x6E
        CHECK(bus->read_word(0x30066D8) == 0);
        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 2); // 2N cycles
        CHECK(bus->read_word(0x30066D8) == 43811);
    }

    SECTION("load") {
        load->load = true;
        bus->write_word(0x30066D8, 11123489);
        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 3); // S + N + I cycles
        CHECK(getr(3) == 47905);
    }
}

TEST_CASE_METHOD(CpuFixture, "SP Relative Load", TAG) {
    InstructionData data =
      SpRelativeLoad{ .word = 0x328, .rd = 1, .load = false };
    SpRelativeLoad* load = std::get_if<SpRelativeLoad>(&data);

    setr(1, 2349505744);
    // sp
    setr(13, 0x3004A8A);

    SECTION("store") {
        // 0x3004A8A + 0x328
        CHECK(bus->read_word(0x3004DB2) == 0);
        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 2); // 2N cycles
        CHECK(bus->read_word(0x3004DB2) == 2349505744);
    }

    SECTION("load") {
        load->load = true;
        bus->write_word(0x3004DB2, 11123489);
        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 3); // S + N + I cycles
        CHECK(getr(1) == 11123489);
    }
}

TEST_CASE_METHOD(CpuFixture, "Load Address", TAG) {
    InstructionData data = LoadAddress{ .word = 808, .rd = 1, .sp = false };
    LoadAddress* load    = std::get_if<LoadAddress>(&data);

    // pc
    setr(15, 336485);
    // sp
    setr(13, 69879977);

    SECTION("PC") {
        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 1); // 1S
        // word align 337293
        CHECK(getr(1) == 337292);
    }

    SECTION("SP") {
        load->sp = true;
        exec(data);
        CHECK(getr(1) == 69880785);
    }
}

TEST_CASE_METHOD(CpuFixture, "Add Offset to Stack Pointer", TAG) {
    InstructionData data       = AddOffsetStackPointer{ .word = 473 };
    AddOffsetStackPointer* add = std::get_if<AddOffsetStackPointer>(&data);

    // sp
    setr(13, 69879977);

    SECTION("positive") {
        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 1); // 1S
        CHECK(getr(13) == 69880450);
    }

    SECTION("negative") {
        add->word = -473;
        exec(data);
        CHECK(getr(13) == 69879504);
    }
}

TEST_CASE_METHOD(CpuFixture, "Push/Pop Registers", TAG) {
    InstructionData data =
      PushPopRegister{ .regs = 0b11010011, .pclr = false, .load = false };
    PushPopRegister* push = std::get_if<PushPopRegister>(&data);

    static constexpr uint8_t alignment = 4;
    static constexpr uint32_t address  = 0x30015AC;

    // registers = 0, 1, 4, 6, 7
    SECTION("push (store)") {

        // populate registers
        setr(0, 237164);
        setr(1, 679785111);
        setr(4, 905895898);
        setr(6, 131313333);
        setr(7, 131);

        auto checker = [this]() {
            // address
            CHECK(bus->read_word(address) == 237164);
            CHECK(bus->read_word(address + alignment) == 679785111);
            CHECK(bus->read_word(address + alignment * 2) == 905895898);
            CHECK(bus->read_word(address + alignment * 3) == 131313333);
            CHECK(bus->read_word(address + alignment * 4) == 131);
        };

        // set stack pointer to top of stack
        setr(13, address + alignment * 5);

        SECTION("without LR") {
            uint32_t cycles = bus->get_cycles();
            exec(data);
            CHECK(bus->get_cycles() == cycles + 6); // 2N + (n-1)S, n = 5
            checker();
            CHECK(getr(13) == address);
        }

        SECTION("with LR") {
            push->pclr = true;
            // populate lr
            setr(14, 999304);
            // add another word on stack (top + 4)
            setr(13, address + alignment * 6);

            uint32_t cycles = bus->get_cycles();
            exec(data);
            CHECK(bus->get_cycles() == cycles + 7); // 2N + nS, n = 5

            CHECK(bus->read_word(address + alignment * 5) == 999304);
            checker();
            CHECK(getr(13) == address);
        }
    }

    SECTION("pop (load)") {
        push->load = true;

        // populate memory
        bus->write_word(address, 237164);
        bus->write_word(address + alignment, 679785111);
        bus->write_word(address + alignment * 2, 905895898);
        bus->write_word(address + alignment * 3, 131313333);
        bus->write_word(address + alignment * 4, 131);

        auto checker = [this]() {
            CHECK(getr(0) == 237164);
            CHECK(getr(1) == 679785111);
            CHECK(getr(2) == 0);
            CHECK(getr(3) == 0);
            CHECK(getr(4) == 905895898);
            CHECK(getr(5) == 0);
            CHECK(getr(6) == 131313333);
            CHECK(getr(7) == 131);

            for (uint8_t i = 0; i < 8; i++) {
                setr(i, 0);
            }
        };

        // set stack pointer to bottom of stack
        setr(13, address);

        SECTION("without PC") {
            uint32_t cycles = bus->get_cycles();
            exec(data);
            CHECK(bus->get_cycles() == cycles + 7); // nS + N + I, n = 5
            checker();
            CHECK(getr(13) == address + alignment * 5);
        }

        SECTION("with PC") {
            push->pclr = true;
            // populate next address
            bus->write_word(address + alignment * 5, 93333912);

            uint32_t cycles = bus->get_cycles();
            exec(data);
            CHECK(bus->get_cycles() == cycles + 10); //(n+2)S + 2N + I, n = 5

            // +4 for flushed pipeline
            CHECK(getr(15) == 93333912 + 4);
            checker();
            CHECK(getr(13) == address + alignment * 6);
        }
    }
}

TEST_CASE_METHOD(CpuFixture, "Multiple Load/Store", TAG) {
    InstructionData data =
      MultipleLoad{ .regs = 0b11010101, .rb = 2, .load = false };
    MultipleLoad* push = std::get_if<MultipleLoad>(&data);
    // registers = 0, 1, 4, 6, 7

    static constexpr uint8_t alignment = 4;
    static constexpr uint32_t address  = 0x30015AC;

    SECTION("store") {

        // populate registers
        setr(0, 237164);
        setr(4, 905895898);
        setr(6, 131313333);
        setr(7, 131);

        // base
        setr(2, address);

        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 6); //(n-1)S + 2N, n = 5

        CHECK(bus->read_word(address) == 237164);
        CHECK(bus->read_word(address + alignment) == address);
        CHECK(bus->read_word(address + alignment * 2) == 905895898);
        CHECK(bus->read_word(address + alignment * 3) == 131313333);
        CHECK(bus->read_word(address + alignment * 4) == 131);

        // write back
        CHECK(getr(2) == address + alignment * 5);
    }

    SECTION("load") {
        push->load = true;

        // populate memory
        bus->write_word(address, 237164);
        bus->write_word(address + alignment, 679785111);
        bus->write_word(address + alignment * 2, 905895898);
        bus->write_word(address + alignment * 3, 131313333);
        bus->write_word(address + alignment * 4, 131);

        // base
        setr(2, address);

        uint32_t cycles = bus->get_cycles();
        exec(data);
        CHECK(bus->get_cycles() == cycles + 7); // nS + N + 1, n = 5

        CHECK(getr(0) == 237164);
        CHECK(getr(1) == 0);
        CHECK(getr(2) == address + alignment * 5); // write back
        CHECK(getr(3) == 0);
        CHECK(getr(4) == 905895898);
        CHECK(getr(5) == 0);
        CHECK(getr(6) == 131313333);
        CHECK(getr(7) == 131);
    }
}

TEST_CASE_METHOD(CpuFixture, "Conditional Branch", TAG) {
    InstructionData data =
      ConditionalBranch{ .offset = -192, .condition = Condition::EQ };
    ConditionalBranch* branch = std::get_if<ConditionalBranch>(&data);

    Psr cpsr = psr();
    cpsr.set_state(State::Thumb);

    setr(15, 4589344);

    SECTION("z") {
        uint32_t cycles = bus->get_cycles();
        // condition is false
        exec(data);
        CHECK(bus->get_cycles() == cycles + 1); // 1S

        // +2 for pc advance
        CHECK(getr(15) == 4589344 + 2);

        cpsr.set_z(true);
        set_psr(cpsr);
        cycles = bus->get_cycles();
        // condition is true
        exec(data);
        CHECK(bus->get_cycles() == cycles + 3); // 2S + N
        // +4 for pipeline flush
        CHECK(getr(15) == 4589156);
    }

    SECTION("c") {
        branch->condition = Condition::CS;
        // condition is false
        exec(data);

        // +2 for pc advance
        CHECK(getr(15) == 4589346);

        cpsr.set_c(true);
        set_psr(cpsr);
        // condition is true
        exec(data);
        // +4 for pipeline flush
        CHECK(getr(15) == 4589156);
    }

    SECTION("n") {
        branch->condition = Condition::MI;
        // condition is false
        exec(data);

        // +2 for pc advance
        CHECK(getr(15) == 4589346);

        cpsr.set_n(true);
        set_psr(cpsr);
        // condition is true
        exec(data);
        // +4 for pipeline flush
        CHECK(getr(15) == 4589156);
    }

    SECTION("v") {
        branch->condition = Condition::VS;
        // condition is false
        exec(data);

        // +2 for pc advance
        CHECK(getr(15) == 4589346);

        cpsr.set_v(true);
        set_psr(cpsr);
        // condition is true
        exec(data);
        // +4 for pipeline flush
        CHECK(getr(15) == 4589156);
    }
}

TEST_CASE_METHOD(CpuFixture, "Software Interrupt", TAG) {
    InstructionData data = SoftwareInterrupt{ .vector = 32 };

    setr(15, 4492);

    uint32_t cycles = bus->get_cycles();
    // condition is true
    exec(data);
    CHECK(bus->get_cycles() == cycles + 3); // 2S + N

    CHECK(psr().raw() == psr(true).raw());
    CHECK(getr(14) == 4490);
    // +4 for flushed pipeline
    CHECK(getr(15) == 36);
    CHECK(psr().state() == State::Arm);
    CHECK(psr().mode() == Mode::Supervisor);
}

TEST_CASE_METHOD(CpuFixture, "Unconditional Branch", TAG) {
    InstructionData data = UnconditionalBranch{ .offset = -920 };

    setr(15, 4589344);

    uint32_t cycles = bus->get_cycles();
    exec(data);
    CHECK(bus->get_cycles() == cycles + 3); // 2S + N

    // +4 for flushed pipeline
    CHECK(getr(15) == 4588428);
}

TEST_CASE_METHOD(CpuFixture, "Long Branch With Link", TAG) {
    InstructionData data =
      LongBranchWithLink{ .offset = 0b10010111110, .low = false };
    LongBranchWithLink* branch = std::get_if<LongBranchWithLink>(&data);

    // high
    setr(15, 4589344);

    uint32_t cycles = bus->get_cycles();
    exec(data);
    CHECK(bus->get_cycles() == cycles + 1); // 1S

    CHECK(getr(14) == 1173280);

    // low
    branch->low = true;

    cycles = bus->get_cycles();
    exec(data);
    CHECK(bus->get_cycles() == cycles + 3); // 2S + N

    // +2 for advancing thumb, then -2 to get the next instruciton of current
    // executing instruction, then set bit 0
    CHECK(getr(14) == 4589343);
    // 1175712 + 4 for flushed pipeline
    CHECK(getr(15) == 1175712);
}
