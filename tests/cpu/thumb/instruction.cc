#include "cpu/thumb/instruction.hh"
#include <catch2/catch_test_macros.hpp>

#define TAG "[thumb][disassembly]"

using namespace matar;
using namespace thumb;

TEST_CASE("Move Shifted Register", TAG) {
    uint16_t raw = 0b0001001101100011;
    Instruction instruction(raw);
    MoveShiftedRegister* lsl = nullptr;

    REQUIRE((lsl = std::get_if<MoveShiftedRegister>(&instruction.data)));
    CHECK(lsl->rd == 3);
    CHECK(lsl->rs == 4);
    CHECK(lsl->offset == 13);
    CHECK(lsl->opcode == ShiftType::ASR);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "ASR R3,R4,#13");

    lsl->opcode = ShiftType::LSR;
    CHECK(instruction.disassemble() == "LSR R3,R4,#13");

    lsl->opcode = ShiftType::LSL;
    CHECK(instruction.disassemble() == "LSL R3,R4,#13");
#endif
}

TEST_CASE("Add/Subtract", TAG) {
    uint16_t raw = 0b0001111101001111;
    Instruction instruction(raw);
    AddSubtract* add = nullptr;

    REQUIRE((add = std::get_if<AddSubtract>(&instruction.data)));
    CHECK(add->rd == 7);
    CHECK(add->rs == 1);
    CHECK(add->offset == 5);
    CHECK(add->opcode == AddSubtract::OpCode::SUB);
    CHECK(add->imm == true);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "SUB R7,R1,#5");

    add->imm = false;
    CHECK(instruction.disassemble() == "SUB R7,R1,R5");

    add->opcode = AddSubtract::OpCode::ADD;
    CHECK(instruction.disassemble() == "ADD R7,R1,R5");
#endif
}

TEST_CASE("Move/Compare/Add/Subtract Immediate", TAG) {
    uint16_t raw = 0b0010111001011011;
    Instruction instruction(raw);
    MovCmpAddSubImmediate* mov = nullptr;

    REQUIRE((mov = std::get_if<MovCmpAddSubImmediate>(&instruction.data)));
    CHECK(mov->offset == 91);
    CHECK(mov->rd == 6);
    CHECK(mov->opcode == MovCmpAddSubImmediate::OpCode::CMP);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "CMP R6,#91");

    mov->opcode = MovCmpAddSubImmediate::OpCode::ADD;
    CHECK(instruction.disassemble() == "ADD R6,#91");

    mov->opcode = MovCmpAddSubImmediate::OpCode::SUB;
    CHECK(instruction.disassemble() == "SUB R6,#91");

    mov->opcode = MovCmpAddSubImmediate::OpCode::MOV;
    CHECK(instruction.disassemble() == "MOV R6,#91");
#endif
}

TEST_CASE("ALU Operations", TAG) {
    uint16_t raw = 0b0100000110011111;
    Instruction instruction(raw);
    AluOperations* alu = nullptr;

    REQUIRE((alu = std::get_if<AluOperations>(&instruction.data)));
    CHECK(alu->rd == 7);
    CHECK(alu->rs == 3);
    CHECK(alu->opcode == AluOperations::OpCode::SBC);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "SBC R7,R3");

#define OPCODE(op)                                                             \
    alu->opcode = AluOperations::OpCode::op;                                   \
    CHECK(instruction.disassemble() == #op " R7,R3");

    OPCODE(AND)
    OPCODE(EOR)
    OPCODE(LSL)
    OPCODE(LSR)
    OPCODE(ASR)
    OPCODE(ADC)
    OPCODE(SBC)
    OPCODE(ROR)
    OPCODE(TST)
    OPCODE(NEG)
    OPCODE(CMP)
    OPCODE(CMN)
    OPCODE(ORR)
    OPCODE(MUL)
    OPCODE(BIC)
    OPCODE(MVN)

#undef OPCODE
#endif
}

TEST_CASE("Hi Register Operations/Branch Exchange", TAG) {
    HiRegisterOperations* hi = nullptr;

    uint16_t raw = 0b0100011000011010;

    SECTION("both lo") {
        Instruction instruction(raw);
        REQUIRE((hi = std::get_if<HiRegisterOperations>(&instruction.data)));

        CHECK(hi->rd == 2);
        CHECK(hi->rs == 3);
    }

    SECTION("hi rd") {
        raw |= 1 << 7;
        Instruction instruction(raw);
        REQUIRE((hi = std::get_if<HiRegisterOperations>(&instruction.data)));

        CHECK(hi->rd == 10);
        CHECK(hi->rs == 3);
    }

    SECTION("hi rs") {
        raw |= 1 << 6;
        Instruction instruction(raw);
        REQUIRE((hi = std::get_if<HiRegisterOperations>(&instruction.data)));

        CHECK(hi->rd == 2);
        CHECK(hi->rs == 11);
    }

    if (hi)
        CHECK(hi->opcode == HiRegisterOperations::OpCode::MOV);

    SECTION("both hi") {
        raw |= 1 << 6;
        raw |= 1 << 7;
        Instruction instruction(raw);
        REQUIRE((hi = std::get_if<HiRegisterOperations>(&instruction.data)));

        CHECK(hi->rd == 10);
        CHECK(hi->rs == 11);
        CHECK(hi->opcode == HiRegisterOperations::OpCode::MOV);

#ifdef DISASSEMBLER
        CHECK(instruction.disassemble() == "MOV R10,R11");

        hi->opcode = HiRegisterOperations::OpCode::ADD;
        CHECK(instruction.disassemble() == "ADD R10,R11");

        hi->opcode = HiRegisterOperations::OpCode::CMP;
        CHECK(instruction.disassemble() == "CMP R10,R11");

        hi->opcode = HiRegisterOperations::OpCode::BX;
        CHECK(instruction.disassemble() == "BX R11");
#endif
    }
}

TEST_CASE("PC Relative Load", TAG) {
    uint16_t raw = 0b0100101011100110;
    Instruction instruction(raw);
    PcRelativeLoad* ldr = nullptr;

    REQUIRE((ldr = std::get_if<PcRelativeLoad>(&instruction.data)));
    // 230 << 2
    CHECK(ldr->word == 920);
    CHECK(ldr->rd == 2);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "LDR R2,[PC,#920]");
#endif
}

TEST_CASE("Load/Store with Register Offset", TAG) {
    uint16_t raw = 0b0101000110011101;
    Instruction instruction(raw);
    LoadStoreRegisterOffset* ldr = nullptr;

    REQUIRE((ldr = std::get_if<LoadStoreRegisterOffset>(&instruction.data)));
    CHECK(ldr->rd == 5);
    CHECK(ldr->rb == 3);
    CHECK(ldr->ro == 6);
    CHECK(ldr->byte == false);
    CHECK(ldr->load == false);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "STR R5,[R3,R6]");

    ldr->byte = true;
    CHECK(instruction.disassemble() == "STRB R5,[R3,R6]");

    ldr->load = true;
    CHECK(instruction.disassemble() == "LDRB R5,[R3,R6]");

    ldr->byte = false;
    CHECK(instruction.disassemble() == "LDR R5,[R3,R6]");
#endif
}

TEST_CASE("Load/Store Sign-Extended Byte/Halfword", TAG) {
    uint16_t raw = 0b0101001110011101;
    Instruction instruction(raw);
    LoadStoreSignExtendedHalfword* ldr = nullptr;

    REQUIRE(
      (ldr = std::get_if<LoadStoreSignExtendedHalfword>(&instruction.data)));
    CHECK(ldr->rd == 5);
    CHECK(ldr->rb == 3);
    CHECK(ldr->ro == 6);
    CHECK(ldr->s == false);
    CHECK(ldr->h == false);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "STRH R5,[R3,R6]");

    ldr->h = true;
    CHECK(instruction.disassemble() == "LDRH R5,[R3,R6]");

    ldr->s = true;
    CHECK(instruction.disassemble() == "LDSH R5,[R3,R6]");

    ldr->h = false;
    CHECK(instruction.disassemble() == "LDSB R5,[R3,R6]");
#endif
}

TEST_CASE("Load/Store with Immediate Offset", TAG) {
    uint16_t raw = 0b0110010110011101;
    Instruction instruction(raw);
    LoadStoreImmediateOffset* ldr = nullptr;

    REQUIRE((ldr = std::get_if<LoadStoreImmediateOffset>(&instruction.data)));
    CHECK(ldr->rd == 5);
    CHECK(ldr->rb == 3);
    // 22 << 4 when byte == false
    CHECK(ldr->offset == 88);
    CHECK(ldr->byte == false);
    CHECK(ldr->load == false);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "STR R5,[R3,#88]");

    ldr->load = true;
    CHECK(instruction.disassemble() == "LDR R5,[R3,#88]");
#endif

    // byte
    raw         = 0b0111010110011101;
    instruction = Instruction(raw);

    INFO(instruction.data.index());
    REQUIRE((ldr = std::get_if<LoadStoreImmediateOffset>(&instruction.data)));
    CHECK(ldr->byte == true);
    CHECK(ldr->offset == 22);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "STRB R5,[R3,#22]");

    ldr->load = true;
    CHECK(instruction.disassemble() == "LDRB R5,[R3,#22]");
#endif
}

TEST_CASE("Load/Store Halfword", TAG) {
    uint16_t raw = 0b1000011010011101;
    Instruction instruction(raw);
    LoadStoreHalfword* ldr = nullptr;

    REQUIRE((ldr = std::get_if<LoadStoreHalfword>(&instruction.data)));
    CHECK(ldr->rd == 5);
    CHECK(ldr->rb == 3);
    // 26 << 1
    CHECK(ldr->offset == 52);
    CHECK(ldr->load == false);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "STRH R5,[R3,#52]");

    ldr->load = true;
    CHECK(instruction.disassemble() == "LDRH R5,[R3,#52]");
#endif
}

TEST_CASE("SP-Relative Load/Store", TAG) {
    uint16_t raw = 0b1001010010011101;
    Instruction instruction(raw);
    SpRelativeLoad* ldr = nullptr;

    REQUIRE((ldr = std::get_if<SpRelativeLoad>(&instruction.data)));
    CHECK(ldr->rd == 4);
    // 157 << 2
    CHECK(ldr->word == 628);
    CHECK(ldr->load == false);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "STR R4,[SP,#628]");

    ldr->load = true;
    CHECK(instruction.disassemble() == "LDR R4,[SP,#628]");
#endif
}

TEST_CASE("Load Adress", TAG) {
    uint16_t raw = 0b1010000110001111;
    Instruction instruction(raw);
    LoadAddress* add = nullptr;

    REQUIRE((add = std::get_if<LoadAddress>(&instruction.data)));
    // 143 << 2
    CHECK(add->word == 572);
    CHECK(add->rd == 1);
    CHECK(add->sp == false);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "ADD R1,PC,#572");

    add->sp = true;
    CHECK(instruction.disassemble() == "ADD R1,SP,#572");
#endif
}

TEST_CASE("Add Offset to Stack Pointer", TAG) {
    uint16_t raw = 0b1011000000100101;
    Instruction instruction(raw);
    AddOffsetStackPointer* add = nullptr;

    REQUIRE((add = std::get_if<AddOffsetStackPointer>(&instruction.data)));
    // 37 << 2
    CHECK(add->word == 148);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "ADD SP,#148");
#endif

    raw         = 0b1011000010100101;
    instruction = Instruction(raw);

    REQUIRE((add = std::get_if<AddOffsetStackPointer>(&instruction.data)));
    CHECK(add->word == -148);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "ADD SP,#-148");
#endif
}

TEST_CASE("Push/Pop Registers", TAG) {
    uint16_t raw = 0b1011010000110101;
    Instruction instruction(raw);
    PushPopRegister* push = nullptr;

    REQUIRE((push = std::get_if<PushPopRegister>(&instruction.data)));
    CHECK(push->regs == 53);
    CHECK(push->pclr == false);
    CHECK(push->load == false);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "PUSH {R0,R2,R4,R5}");

    push->pclr = true;
    CHECK(instruction.disassemble() == "PUSH {R0,R2,R4,R5,LR}");

    push->load = true;
    CHECK(instruction.disassemble() == "POP {R0,R2,R4,R5,PC}");

    push->pclr = false;
    CHECK(instruction.disassemble() == "POP {R0,R2,R4,R5}");
#endif
}

TEST_CASE("Multiple Load/Store", TAG) {
    uint16_t raw = 0b1100011001100101;
    Instruction instruction(raw);
    MultipleLoad* ldm = nullptr;

    REQUIRE((ldm = std::get_if<MultipleLoad>(&instruction.data)));
    CHECK(ldm->regs == 101);
    CHECK(ldm->rb == 6);
    CHECK(ldm->load == false);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "STMIA R6!,{R0,R2,R5,R6}");

    ldm->load = true;
    CHECK(instruction.disassemble() == "LDMIA R6!,{R0,R2,R5,R6}");
#endif
}

TEST_CASE("Conditional Branch", TAG) {
    uint16_t raw = 0b1101100110110100;
    Instruction instruction(raw);
    ConditionalBranch* b = nullptr;

    REQUIRE((b = std::get_if<ConditionalBranch>(&instruction.data)));
    // (-76 << 1)
    CHECK(b->offset == -152);
    CHECK(b->condition == Condition::LS);

#ifdef DISASSEMBLER
    // take prefetch into account
    // offset + 4 = -152 + 4
    CHECK(instruction.disassemble() == "BLS #-148");
#endif
}

TEST_CASE("SoftwareInterrupt") {
    uint16_t raw = 0b1101111100110011;
    Instruction instruction(raw);
    SoftwareInterrupt* swi = nullptr;

    REQUIRE((swi = std::get_if<SoftwareInterrupt>(&instruction.data)));

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "SWI 51");
#endif
}

TEST_CASE("Unconditional Branch") {
    uint16_t raw = 0b1110011100110011;
    Instruction instruction(raw);
    UnconditionalBranch* b = nullptr;

    REQUIRE((b = std::get_if<UnconditionalBranch>(&instruction.data)));
    // (2147483443 << 1)
    REQUIRE(b->offset == -410);

#ifdef DISASSEMBLER
    // take prefetch into account
    // offset + 4 = -410 + 4
    CHECK(instruction.disassemble() == "B #-406");
#endif
}

TEST_CASE("Long Branch with link") {
    uint16_t raw = 0b1111110011101100;
    Instruction instruction(raw);
    LongBranchWithLink* bl = nullptr;

    REQUIRE((bl = std::get_if<LongBranchWithLink>(&instruction.data)));
    // 1260 << 1
    CHECK(bl->offset == 1260);
    CHECK(bl->low == true);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "BL #1260");

    bl->low = false;
    CHECK(instruction.disassemble() == "BLH #1260");
#endif
}

#undef TAG
