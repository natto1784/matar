#include "cpu/arm/instruction.hh"
#include <catch2/catch_test_macros.hpp>

#define TAG "[arm][disassembly]"

using namespace matar;
using namespace arm;

TEST_CASE("Branch and Exchange", TAG) {
    uint32_t raw = 0b11000001001011111111111100011010;
    Instruction instruction(raw);
    BranchAndExchange* bx = nullptr;

    REQUIRE((bx = std::get_if<BranchAndExchange>(&instruction.data)));
    CHECK(instruction.condition == Condition::GT);

    CHECK(bx->rn == 10);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "BXGT R10");
#endif
}

TEST_CASE("Branch", TAG) {
    uint32_t raw = 0b11101011100001010111111111000011;
    Instruction instruction(raw);
    Branch* b = nullptr;

    REQUIRE((b = std::get_if<Branch>(&instruction.data)));
    CHECK(instruction.condition == Condition::AL);

    // last 24 bits = 8748995
    // (8748995 << 8) >> 6 sign extended = 0xFE15FF0C
    // Also +8 since PC is two instructions ahead
    CHECK(b->offset == 0xFE15FF14);
    CHECK(b->link == true);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "BL 0xFE15FF14");

    b->link = false;
    CHECK(instruction.disassemble() == "B 0xFE15FF14");
#endif
}

TEST_CASE("Multiply", TAG) {
    uint32_t raw = 0b00000000001110101110111110010000;
    Instruction instruction(raw);
    Multiply* mul = nullptr;

    REQUIRE((mul = std::get_if<Multiply>(&instruction.data)));
    CHECK(instruction.condition == Condition::EQ);

    CHECK(mul->rm == 0);
    CHECK(mul->rs == 15);
    CHECK(mul->rn == 14);
    CHECK(mul->rd == 10);
    CHECK(mul->acc == true);
    CHECK(mul->set == true);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "MLAEQS R10,R0,R15,R14");

    mul->acc = false;
    mul->set = false;
    CHECK(instruction.disassemble() == "MULEQ R10,R0,R15");
#endif
}

TEST_CASE("Multiply Long", TAG) {
    uint32_t raw = 0b00010000100111100111011010010010;
    Instruction instruction(raw);
    MultiplyLong* mull = nullptr;

    REQUIRE((mull = std::get_if<MultiplyLong>(&instruction.data)));
    CHECK(instruction.condition == Condition::NE);

    CHECK(mull->rm == 2);
    CHECK(mull->rs == 6);
    CHECK(mull->rdlo == 7);
    CHECK(mull->rdhi == 14);
    CHECK(mull->acc == false);
    CHECK(mull->set == true);
    CHECK(mull->uns == true);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "UMULLNES R7,R14,R2,R6");

    mull->acc = true;
    CHECK(instruction.disassemble() == "UMLALNES R7,R14,R2,R6");

    mull->uns = false;
    mull->set = false;
    CHECK(instruction.disassemble() == "SMLALNE R7,R14,R2,R6");
#endif
}

TEST_CASE("Undefined", TAG) {
    // notice how this is the same as single data transfer except the shift
    // is now a register based shift
    uint32_t raw = 0b11100111101000101010111100010110;
    Instruction instruction(raw);

    CHECK(instruction.condition == Condition::AL);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "UND");
#endif
}

TEST_CASE("Single Data Swap", TAG) {
    uint32_t raw = 0b10100001000010010101000010010110;
    Instruction instruction(raw);
    SingleDataSwap* swp = nullptr;

    REQUIRE((swp = std::get_if<SingleDataSwap>(&instruction.data)));
    CHECK(instruction.condition == Condition::GE);

    CHECK(swp->rm == 6);
    CHECK(swp->rd == 5);
    CHECK(swp->rn == 9);
    CHECK(swp->byte == false);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "SWPGE R5,R6,[R9]");

    swp->byte = true;
    CHECK(instruction.disassemble() == "SWPGEB R5,R6,[R9]");
#endif
}

TEST_CASE("Single Data Transfer", TAG) {
    uint32_t raw = 0b11100111101000101010111100000110;
    Instruction instruction(raw);
    SingleDataTransfer* ldr = nullptr;
    Shift* shift            = nullptr;

    REQUIRE((ldr = std::get_if<SingleDataTransfer>(&instruction.data)));
    CHECK(instruction.condition == Condition::AL);

    REQUIRE((shift = std::get_if<Shift>(&ldr->offset)));
    CHECK(shift->rm == 6);
    CHECK(shift->data.immediate == true);
    CHECK(shift->data.type == ShiftType::LSL);
    CHECK(shift->data.operand == 30);
    CHECK(ldr->rd == 10);
    CHECK(ldr->rn == 2);
    CHECK(ldr->load == false);
    CHECK(ldr->write == true);
    CHECK(ldr->byte == false);
    CHECK(ldr->up == true);
    CHECK(ldr->pre == true);

#ifdef DISASSEMBLER
    ldr->load        = true;
    ldr->byte        = true;
    ldr->write       = false;
    shift->data.type = ShiftType::ROR;
    CHECK(instruction.disassemble() == "LDRB R10,[R2,+R6,ROR #30]");

    ldr->up  = false;
    ldr->pre = false;
    CHECK(instruction.disassemble() == "LDRB R10,[R2],-R6,ROR #30");

    ldr->offset = static_cast<uint16_t>(9023);
    CHECK(instruction.disassemble() == "LDRB R10,[R2],-#9023");

    ldr->pre = true;
    CHECK(instruction.disassemble() == "LDRB R10,[R2,-#9023]");
#endif
}

TEST_CASE("Halfword Transfer", TAG) {
    uint32_t raw = 0b00110001101011110010000010110110;
    Instruction instruction(raw);
    HalfwordTransfer* ldr = nullptr;

    REQUIRE((ldr = std::get_if<HalfwordTransfer>(&instruction.data)));
    CHECK(instruction.condition == Condition::CC);

    // offset is not immediate
    CHECK(ldr->imm == 0);
    // hence this offset is a register number (rm)
    CHECK(ldr->offset == 6);
    CHECK(ldr->half == true);
    CHECK(ldr->sign == false);
    CHECK(ldr->rd == 2);
    CHECK(ldr->rn == 15);
    CHECK(ldr->load == false);
    CHECK(ldr->write == true);
    CHECK(ldr->up == true);
    CHECK(ldr->pre == true);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "STRCCH R2,[R15,+R6]!");

    ldr->pre  = false;
    ldr->load = true;
    ldr->sign = true;
    ldr->up   = false;

    CHECK(instruction.disassemble() == "LDRCCSH R2,[R15],-R6");

    ldr->half = false;
    CHECK(instruction.disassemble() == "LDRCCSB R2,[R15],-R6");

    ldr->load = false;
    // not a register anymore
    ldr->imm    = 1;
    ldr->offset = 90;
    CHECK(instruction.disassemble() == "STRCCSB R2,[R15],-#90");
#endif
}

TEST_CASE("Block Data Transfer", TAG) {
    uint32_t raw = 0b10011001010101110100000101101101;
    Instruction instruction(raw);
    BlockDataTransfer* ldm = nullptr;

    REQUIRE((ldm = std::get_if<BlockDataTransfer>(&instruction.data)));
    CHECK(instruction.condition == Condition::LS);

    {
        uint16_t regs = 0;
        regs |= 1 << 0;
        regs |= 1 << 2;
        regs |= 1 << 3;
        regs |= 1 << 5;
        regs |= 1 << 6;
        regs |= 1 << 8;
        regs |= 1 << 14;

        CHECK(ldm->regs == regs);
    }

    CHECK(ldm->rn == 7);
    CHECK(ldm->load == true);
    CHECK(ldm->write == false);
    CHECK(ldm->s == true);
    CHECK(ldm->up == false);
    CHECK(ldm->pre == true);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "LDMLSDB R7,{R0,R2,R3,R5,R6,R8,R14}^");

    ldm->write = true;
    ldm->s     = false;
    ldm->up    = true;

    CHECK(instruction.disassemble() == "LDMLSIB R7!,{R0,R2,R3,R5,R6,R8,R14}");

    ldm->regs &= ~(1 << 6);
    ldm->regs &= ~(1 << 3);
    ldm->regs &= ~(1 << 8);
    ldm->load = false;
    ldm->pre  = false;

    CHECK(instruction.disassemble() == "STMLSIA R7!,{R0,R2,R5,R14}");
#endif
}

TEST_CASE("PSR Transfer", TAG) {
    PsrTransfer* msr = nullptr;

    SECTION("MRS") {
        uint32_t raw = 0b01000001010011111010000000000000;
        Instruction instruction(raw);
        PsrTransfer* mrs = nullptr;

        REQUIRE((mrs = std::get_if<PsrTransfer>(&instruction.data)));
        CHECK(instruction.condition == Condition::MI);

        CHECK(mrs->type == PsrTransfer::Type::Mrs);
        // Operand is a register in the case of MRS (PSR -> Register)
        CHECK(mrs->operand == 10);
        CHECK(mrs->spsr == true);

#ifdef DISASSEMBLER
        CHECK(instruction.disassemble() == "MRSMI R10,SPSR_all");
#endif
    }

    SECTION("MSR") {
        uint32_t raw = 0b11100001001010011111000000001000;
        Instruction instruction(raw);
        PsrTransfer* msr = nullptr;

        REQUIRE((msr = std::get_if<PsrTransfer>(&instruction.data)));
        CHECK(instruction.condition == Condition::AL);

        CHECK(msr->type == PsrTransfer::Type::Msr);
        // Operand is a register in the case of MSR (Register -> PSR)
        CHECK(msr->operand == 8);
        CHECK(msr->spsr == false);

#ifdef DISASSEMBLER
        CHECK(instruction.disassemble() == "MSR CPSR_all,R8");
#endif
    }

    SECTION("MSR_flg with register operand") {
        uint32_t raw = 0b01100001001010001111000000001000;
        Instruction instruction(raw);

        REQUIRE((msr = std::get_if<PsrTransfer>(&instruction.data)));
        CHECK(instruction.condition == Condition::VS);

        CHECK(msr->type == PsrTransfer::Type::Msr_flg);
        CHECK(msr->imm == 0);
        CHECK(msr->operand == 8);
        CHECK(msr->spsr == false);

#ifdef DISASSEMBLER
        CHECK(instruction.disassemble() == "MSRVS CPSR_flg,R8");
#endif
    }

    SECTION("MSR_flg with immediate operand") {
        uint32_t raw = 0b11100011011010001111011101101000;
        Instruction instruction(raw);

        REQUIRE((msr = std::get_if<PsrTransfer>(&instruction.data)));
        CHECK(instruction.condition == Condition::AL);

        CHECK(msr->type == PsrTransfer::Type::Msr_flg);
        CHECK(msr->imm == 1);

        // 104 (32 bits) rotated by 2 * 7
        CHECK(msr->operand == 27262976);
        CHECK(msr->spsr == true);

#ifdef DISASSEMBLER
        CHECK(instruction.disassemble() == "MSR SPSR_flg,#27262976");
#endif
    }
}

TEST_CASE("Data Processing", TAG) {
    using OpCode = DataProcessing::OpCode;

    uint32_t raw = 0b11100000000111100111101101100001;
    Instruction instruction(raw);
    DataProcessing* alu = nullptr;
    Shift* shift        = nullptr;

    REQUIRE((alu = std::get_if<DataProcessing>(&instruction.data)));
    CHECK(instruction.condition == Condition::AL);

    // operand 2 is a shifted register
    REQUIRE((shift = std::get_if<Shift>(&alu->operand)));
    CHECK(shift->rm == 1);
    CHECK(shift->data.immediate == true);
    CHECK(shift->data.type == ShiftType::ROR);
    CHECK(shift->data.operand == 22);

    CHECK(alu->rd == 7);
    CHECK(alu->rn == 14);
    CHECK(alu->set == true);
    CHECK(alu->opcode == OpCode::AND);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "ANDS R7,R14,R1,ROR #22");

    shift->data.immediate = false;
    shift->data.operand   = 2;
    alu->set              = false;

    CHECK(instruction.disassemble() == "AND R7,R14,R1,ROR R2");

    alu->operand = static_cast<uint32_t>(3300012);
    CHECK(instruction.disassemble() == "AND R7,R14,#3300012");

    SECTION("set-only operations") {
        alu->set = true;

        alu->opcode = OpCode::TST;
        CHECK(instruction.disassemble() == "TST R14,#3300012");

        alu->opcode = OpCode::TEQ;
        CHECK(instruction.disassemble() == "TEQ R14,#3300012");

        alu->opcode = OpCode::CMP;
        CHECK(instruction.disassemble() == "CMP R14,#3300012");

        alu->opcode = OpCode::CMN;
        CHECK(instruction.disassemble() == "CMN R14,#3300012");
    }

    SECTION("destination operations") {
        alu->opcode = OpCode::EOR;
        CHECK(instruction.disassemble() == "EOR R7,R14,#3300012");

        alu->opcode = OpCode::SUB;
        CHECK(instruction.disassemble() == "SUB R7,R14,#3300012");

        alu->opcode = OpCode::RSB;
        CHECK(instruction.disassemble() == "RSB R7,R14,#3300012");

        alu->opcode = OpCode::SUB;
        CHECK(instruction.disassemble() == "SUB R7,R14,#3300012");

        alu->opcode = OpCode::ADC;
        CHECK(instruction.disassemble() == "ADC R7,R14,#3300012");

        alu->opcode = OpCode::SBC;
        CHECK(instruction.disassemble() == "SBC R7,R14,#3300012");

        alu->opcode = OpCode::RSC;
        CHECK(instruction.disassemble() == "RSC R7,R14,#3300012");

        alu->opcode = OpCode::ORR;
        CHECK(instruction.disassemble() == "ORR R7,R14,#3300012");

        alu->opcode = OpCode::MOV;
        CHECK(instruction.disassemble() == "MOV R7,#3300012");

        alu->opcode = OpCode::BIC;
        CHECK(instruction.disassemble() == "BIC R7,R14,#3300012");

        alu->opcode = OpCode::MVN;
        CHECK(instruction.disassemble() == "MVN R7,#3300012");
    }
#endif
}

TEST_CASE("Coprocessor Data Transfer", TAG) {
    uint32_t raw = 0b10101101101001011111000101000110;
    Instruction instruction(raw);
    CoprocessorDataTransfer* ldc = nullptr;

    REQUIRE((ldc = std::get_if<CoprocessorDataTransfer>(&instruction.data)));
    CHECK(instruction.condition == Condition::GE);

    CHECK(ldc->offset == 70);
    CHECK(ldc->cpn == 1);
    CHECK(ldc->crd == 15);
    CHECK(ldc->rn == 5);
    CHECK(ldc->load == false);
    CHECK(ldc->write == true);
    CHECK(ldc->len == false);
    CHECK(ldc->up == true);
    CHECK(ldc->pre == true);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "STCGE p1,c15,[R5,#70]!");

    ldc->load  = true;
    ldc->pre   = false;
    ldc->write = false;
    ldc->len   = true;

    CHECK(instruction.disassemble() == "LDCGEL p1,c15,[R5],#70");
#endif
}

TEST_CASE("Coprocessor Operand Operation", TAG) {
    uint32_t raw = 0b11101110101001011111000101000110;
    Instruction instruction(raw);
    CoprocessorDataOperation* cdp = nullptr;

    REQUIRE((cdp = std::get_if<CoprocessorDataOperation>(&instruction.data)));
    CHECK(instruction.condition == Condition::AL);

    CHECK(cdp->crm == 6);
    CHECK(cdp->cp == 2);
    CHECK(cdp->cpn == 1);
    CHECK(cdp->crd == 15);
    CHECK(cdp->crn == 5);
    CHECK(cdp->cp_opc == 10);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "CDP p1,10,c15,c5,c6,2");
#endif
}

TEST_CASE("Coprocessor Register Transfer", TAG) {
    uint32_t raw = 0b11101110101001011111000101010110;
    Instruction instruction(raw);
    CoprocessorRegisterTransfer* mrc = nullptr;

    REQUIRE(
      (mrc = std::get_if<CoprocessorRegisterTransfer>(&instruction.data)));
    CHECK(instruction.condition == Condition::AL);

    CHECK(mrc->crm == 6);
    CHECK(mrc->cp == 2);
    CHECK(mrc->cpn == 1);
    CHECK(mrc->rd == 15);
    CHECK(mrc->crn == 5);
    CHECK(mrc->load == false);
    CHECK(mrc->cp_opc == 5);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "MCR p1,5,R15,c5,c6,2");
#endif
}

TEST_CASE("Software Interrupt", TAG) {
    uint32_t raw = 0b00001111101010101010101010101010;
    Instruction instruction(raw);

    CHECK(instruction.condition == Condition::EQ);

#ifdef DISASSEMBLER
    CHECK(instruction.disassemble() == "SWIEQ");
#endif
}

#undef TAG
