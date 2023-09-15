#include "cpu/instruction.hh"
#include "cpu/utility.hh"
#include <catch2/catch_test_macros.hpp>
#include <iostream>

static constexpr auto TAG = "disassembler";

using namespace arm;

TEST_CASE("Branch and Exchange", TAG) {
    uint32_t raw = 0b11000001001011111111111100011010;
    ArmInstruction instruction(raw);
    BranchAndExchange* bx = nullptr;

    REQUIRE((bx = std::get_if<BranchAndExchange>(&instruction.data)));
    REQUIRE(instruction.condition == Condition::GT);

    REQUIRE(bx->rn == 10);

    REQUIRE(instruction.disassemble() == "BXGT R10");
}

TEST_CASE("Branch", TAG) {
    uint32_t raw = 0b11101011100001010111111111000011;
    ArmInstruction instruction(raw);
    Branch* b = nullptr;

    REQUIRE((b = std::get_if<Branch>(&instruction.data)));
    REQUIRE(instruction.condition == Condition::AL);

    // last 24 bits = 8748995
    // (8748995 << 8) >> 6 sign extended = 0xFE15FF0C
    // Also +8 since PC is two instructions ahead
    REQUIRE(b->offset == 0xFE15FF14);
    REQUIRE(b->link == true);

    REQUIRE(instruction.disassemble() == "BL 0xFE15FF14");

    b->link = false;
    REQUIRE(instruction.disassemble() == "B 0xFE15FF14");
}

TEST_CASE("Multiply", TAG) {
    uint32_t raw = 0b00000000001110101110111110010000;
    ArmInstruction instruction(raw);
    Multiply* mul = nullptr;

    REQUIRE((mul = std::get_if<Multiply>(&instruction.data)));
    REQUIRE(instruction.condition == Condition::EQ);

    REQUIRE(mul->rm == 0);
    REQUIRE(mul->rs == 15);
    REQUIRE(mul->rn == 14);
    REQUIRE(mul->rd == 10);
    REQUIRE(mul->acc == true);
    REQUIRE(mul->set == true);

    REQUIRE(instruction.disassemble() == "MLAEQS R10,R0,R15,R14");

    mul->acc = false;
    mul->set = false;
    REQUIRE(instruction.disassemble() == "MULEQ R10,R0,R15");
}

TEST_CASE("Multiply Long", TAG) {
    uint32_t raw = 0b00010000100111100111011010010010;
    ArmInstruction instruction(raw);
    MultiplyLong* mull = nullptr;

    REQUIRE((mull = std::get_if<MultiplyLong>(&instruction.data)));
    REQUIRE(instruction.condition == Condition::NE);

    REQUIRE(mull->rm == 2);
    REQUIRE(mull->rs == 6);
    REQUIRE(mull->rdlo == 7);
    REQUIRE(mull->rdhi == 14);
    REQUIRE(mull->acc == false);
    REQUIRE(mull->set == true);
    REQUIRE(mull->uns == false);

    REQUIRE(instruction.disassemble() == "SMULLNES R7,R14,R2,R6");

    mull->acc = true;
    REQUIRE(instruction.disassemble() == "SMLALNES R7,R14,R2,R6");

    mull->uns = true;
    mull->set = false;
    REQUIRE(instruction.disassemble() == "UMLALNE R7,R14,R2,R6");
}

TEST_CASE("Single Data Swap", TAG) {
    uint32_t raw = 0b10100001000010010101000010010110;
    ArmInstruction instruction(raw);
    SingleDataSwap* swp = nullptr;

    REQUIRE((swp = std::get_if<SingleDataSwap>(&instruction.data)));
    REQUIRE(instruction.condition == Condition::GE);

    REQUIRE(swp->rm == 6);
    REQUIRE(swp->rd == 5);
    REQUIRE(swp->rn == 9);
    REQUIRE(swp->byte == false);

    REQUIRE(instruction.disassemble() == "SWPGE R5,R6,[R9]");

    swp->byte = true;
    REQUIRE(instruction.disassemble() == "SWPGEB R5,R6,[R9]");
}

TEST_CASE("Single Data Transfer", TAG) {
    uint32_t raw = 0b11100111101000101010111100000110;
    ArmInstruction instruction(raw);
    SingleDataTransfer* ldr = nullptr;
    Shift* shift            = nullptr;

    REQUIRE((ldr = std::get_if<SingleDataTransfer>(&instruction.data)));
    REQUIRE(instruction.condition == Condition::AL);

    REQUIRE((shift = std::get_if<Shift>(&ldr->offset)));
    REQUIRE(shift->rm == 6);
    REQUIRE(shift->data.immediate == true);
    REQUIRE(shift->data.type == ShiftType::LSL);
    REQUIRE(shift->data.operand == 30);
    REQUIRE(ldr->rd == 10);
    REQUIRE(ldr->rn == 2);
    REQUIRE(ldr->load == false);
    REQUIRE(ldr->write == true);
    REQUIRE(ldr->byte == false);
    REQUIRE(ldr->up == true);
    REQUIRE(ldr->pre == true);

    ldr->load        = true;
    ldr->byte        = true;
    ldr->write       = false;
    shift->data.type = ShiftType::ROR;
    REQUIRE(instruction.disassemble() == "LDRB R10,[R2,+R6,ROR #30]");

    ldr->up  = false;
    ldr->pre = false;
    REQUIRE(instruction.disassemble() == "LDRB R10,[R2],-R6,ROR #30");

    ldr->offset = static_cast<uint16_t>(9023);
    REQUIRE(instruction.disassemble() == "LDRB R10,[R2],-#9023");

    ldr->pre = true;
    REQUIRE(instruction.disassemble() == "LDRB R10,[R2,-#9023]");
}

TEST_CASE("Halfword Transfer", TAG) {
    uint32_t raw = 0b00110001101011110010000010110110;
    ArmInstruction instruction(raw);
    HalfwordTransfer* ldr = nullptr;

    REQUIRE((ldr = std::get_if<HalfwordTransfer>(&instruction.data)));
    REQUIRE(instruction.condition == Condition::CC);

    // offset is not immediate
    REQUIRE(ldr->imm == 0);
    // hence this offset is a register number (rm)
    REQUIRE(ldr->offset == 6);
    REQUIRE(ldr->half == true);
    REQUIRE(ldr->sign == false);
    REQUIRE(ldr->rd == 2);
    REQUIRE(ldr->rn == 15);
    REQUIRE(ldr->load == false);
    REQUIRE(ldr->write == true);
    REQUIRE(ldr->up == true);
    REQUIRE(ldr->pre == true);

    REQUIRE(instruction.disassemble() == "STRCCH R2,[R15,+R6]!");

    ldr->pre  = false;
    ldr->load = true;
    ldr->sign = true;
    ldr->up   = false;

    REQUIRE(instruction.disassemble() == "LDRCCSH R2,[R15],-R6");

    ldr->half = false;
    REQUIRE(instruction.disassemble() == "LDRCCSB R2,[R15],-R6");

    // not a register anymore
    ldr->load   = false;
    ldr->imm    = 1;
    ldr->offset = 90;
    REQUIRE(instruction.disassemble() == "STRCCSB R2,[R15],-#90");
}

TEST_CASE("Undefined", TAG) {
    // notice how this is the same as single data transfer except the shift is
    // now a register based shift
    uint32_t raw = 0b11100111101000101010111100010110;

    REQUIRE(ArmInstruction(raw).disassemble() == "UNDEFINED");

    raw = 0b11100110000000000000000000010000;
    REQUIRE(ArmInstruction(raw).disassemble() == "UNDEFINED");
}
