#include "cpu/arm/instruction.hh"
#include "cpu/utility.hh"
#include <catch2/catch_test_macros.hpp>

#define TAG "disassembler"

using namespace arm;

TEST_CASE("Branch and Exchange", TAG) {
    uint32_t raw = 0b11000001001011111111111100011010;
    Instruction instruction(raw);
    BranchAndExchange* bx = nullptr;

    REQUIRE((bx = std::get_if<BranchAndExchange>(&instruction.data)));
    REQUIRE(instruction.condition == Condition::GT);

    REQUIRE(bx->rn == 10);

    REQUIRE(instruction.disassemble() == "BXGT R10");
}

TEST_CASE("Branch", TAG) {
    uint32_t raw = 0b11101011100001010111111111000011;
    Instruction instruction(raw);
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
    Instruction instruction(raw);
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
    Instruction instruction(raw);
    MultiplyLong* mull = nullptr;

    REQUIRE((mull = std::get_if<MultiplyLong>(&instruction.data)));
    REQUIRE(instruction.condition == Condition::NE);

    REQUIRE(mull->rm == 2);
    REQUIRE(mull->rs == 6);
    REQUIRE(mull->rdlo == 7);
    REQUIRE(mull->rdhi == 14);
    REQUIRE(mull->acc == false);
    REQUIRE(mull->set == true);
    REQUIRE(mull->uns == true);

    REQUIRE(instruction.disassemble() == "UMULLNES R7,R14,R2,R6");

    mull->acc = true;
    REQUIRE(instruction.disassemble() == "UMLALNES R7,R14,R2,R6");

    mull->uns = false;
    mull->set = false;
    REQUIRE(instruction.disassemble() == "SMLALNE R7,R14,R2,R6");
}

TEST_CASE("Undefined", TAG) {
    // notice how this is the same as single data transfer except the shift
    // is now a register based shift
    uint32_t raw = 0b11100111101000101010111100010110;
    Instruction instruction(raw);

    REQUIRE(instruction.condition == Condition::AL);
    REQUIRE(instruction.disassemble() == "UND");
}

TEST_CASE("Single Data Swap", TAG) {
    uint32_t raw = 0b10100001000010010101000010010110;
    Instruction instruction(raw);
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
    Instruction instruction(raw);
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
    Instruction instruction(raw);
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

    ldr->load = false;
    // not a register anymore
    ldr->imm    = 1;
    ldr->offset = 90;
    REQUIRE(instruction.disassemble() == "STRCCSB R2,[R15],-#90");
}

TEST_CASE("Block Data Transfer", TAG) {
    uint32_t raw = 0b10011001010101110100000101101101;
    Instruction instruction(raw);
    BlockDataTransfer* ldm = nullptr;

    REQUIRE((ldm = std::get_if<BlockDataTransfer>(&instruction.data)));
    REQUIRE(instruction.condition == Condition::LS);

    {
        uint16_t regs = 0;
        regs |= 1 << 0;
        regs |= 1 << 2;
        regs |= 1 << 3;
        regs |= 1 << 5;
        regs |= 1 << 6;
        regs |= 1 << 8;
        regs |= 1 << 14;

        REQUIRE(ldm->regs == regs);
    }

    REQUIRE(ldm->rn == 7);
    REQUIRE(ldm->load == true);
    REQUIRE(ldm->write == false);
    REQUIRE(ldm->s == true);
    REQUIRE(ldm->up == false);
    REQUIRE(ldm->pre == true);

    REQUIRE(instruction.disassemble() == "LDMLSDB R7,{R0,R2,R3,R5,R6,R8,R14}^");

    ldm->write = true;
    ldm->s     = false;
    ldm->up    = true;

    REQUIRE(instruction.disassemble() == "LDMLSIB R7!,{R0,R2,R3,R5,R6,R8,R14}");

    ldm->regs &= ~(1 << 6);
    ldm->regs &= ~(1 << 3);
    ldm->regs &= ~(1 << 8);
    ldm->load = false;
    ldm->pre  = false;

    REQUIRE(instruction.disassemble() == "STMLSIA R7!,{R0,R2,R5,R14}");
}

TEST_CASE("PSR Transfer", TAG) {
    PsrTransfer* msr = nullptr;

    SECTION("MRS") {
        uint32_t raw = 0b01000001010011111010000000000000;
        Instruction instruction(raw);
        PsrTransfer* mrs = nullptr;

        REQUIRE((mrs = std::get_if<PsrTransfer>(&instruction.data)));
        REQUIRE(instruction.condition == Condition::MI);

        REQUIRE(mrs->type == PsrTransfer::Type::Mrs);
        // Operand is a register in the case of MRS (PSR -> Register)
        REQUIRE(mrs->operand == 10);
        REQUIRE(mrs->spsr == true);

        REQUIRE(instruction.disassemble() == "MRSMI R10,SPSR_all");
    }

    SECTION("MSR") {
        uint32_t raw = 0b11100001001010011111000000001000;
        Instruction instruction(raw);
        PsrTransfer* msr = nullptr;

        REQUIRE((msr = std::get_if<PsrTransfer>(&instruction.data)));
        REQUIRE(instruction.condition == Condition::AL);

        REQUIRE(msr->type == PsrTransfer::Type::Msr);
        // Operand is a register in the case of MSR (Register -> PSR)
        REQUIRE(msr->operand == 8);
        REQUIRE(msr->spsr == false);

        REQUIRE(instruction.disassemble() == "MSR CPSR_all,R8");
    }

    SECTION("MSR_flg with register operand") {
        uint32_t raw = 0b01100001001010001111000000001000;
        Instruction instruction(raw);

        REQUIRE((msr = std::get_if<PsrTransfer>(&instruction.data)));
        REQUIRE(instruction.condition == Condition::VS);

        REQUIRE(msr->type == PsrTransfer::Type::Msr_flg);
        REQUIRE(msr->imm == 0);
        REQUIRE(msr->operand == 8);
        REQUIRE(msr->spsr == false);

        REQUIRE(instruction.disassemble() == "MSRVS CPSR_flg,R8");
    }

    SECTION("MSR_flg with immediate operand") {
        uint32_t raw = 0b11100011011010001111011101101000;
        Instruction instruction(raw);

        REQUIRE((msr = std::get_if<PsrTransfer>(&instruction.data)));
        REQUIRE(instruction.condition == Condition::AL);

        REQUIRE(msr->type == PsrTransfer::Type::Msr_flg);
        REQUIRE(msr->imm == 1);

        // 104 (32 bits) rotated by 2 * 7
        REQUIRE(msr->operand == 27262976);
        REQUIRE(msr->spsr == true);

        REQUIRE(instruction.disassemble() == "MSR SPSR_flg,#27262976");
    }
}

TEST_CASE("Data Processing", TAG) {
    uint32_t raw = 0b11100000000111100111101101100001;
    Instruction instruction(raw);
    DataProcessing* alu = nullptr;
    Shift* shift        = nullptr;

    REQUIRE((alu = std::get_if<DataProcessing>(&instruction.data)));
    REQUIRE(instruction.condition == Condition::AL);

    // operand 2 is a shifted register
    REQUIRE((shift = std::get_if<Shift>(&alu->operand)));
    REQUIRE(shift->rm == 1);
    REQUIRE(shift->data.immediate == true);
    REQUIRE(shift->data.type == ShiftType::ROR);
    REQUIRE(shift->data.operand == 22);

    REQUIRE(alu->rd == 7);
    REQUIRE(alu->rn == 14);
    REQUIRE(alu->set == true);
    REQUIRE(alu->opcode == OpCode::AND);

    REQUIRE(instruction.disassemble() == "ANDS R7,R14,R1,ROR #22");

    shift->data.immediate = false;
    shift->data.operand   = 2;
    alu->set              = false;

    REQUIRE(instruction.disassemble() == "AND R7,R14,R1,ROR R2");

    alu->operand = static_cast<uint32_t>(3300012);
    REQUIRE(instruction.disassemble() == "AND R7,R14,#3300012");

    SECTION("set-only operations") {
        alu->set = true;

        alu->opcode = OpCode::TST;
        REQUIRE(instruction.disassemble() == "TST R14,#3300012");

        alu->opcode = OpCode::TEQ;
        REQUIRE(instruction.disassemble() == "TEQ R14,#3300012");

        alu->opcode = OpCode::CMP;
        REQUIRE(instruction.disassemble() == "CMP R14,#3300012");

        alu->opcode = OpCode::CMN;
        REQUIRE(instruction.disassemble() == "CMN R14,#3300012");
    }

    SECTION("destination operations") {
        alu->opcode = OpCode::EOR;
        REQUIRE(instruction.disassemble() == "EOR R7,R14,#3300012");

        alu->opcode = OpCode::SUB;
        REQUIRE(instruction.disassemble() == "SUB R7,R14,#3300012");

        alu->opcode = OpCode::RSB;
        REQUIRE(instruction.disassemble() == "RSB R7,R14,#3300012");

        alu->opcode = OpCode::SUB;
        REQUIRE(instruction.disassemble() == "SUB R7,R14,#3300012");

        alu->opcode = OpCode::ADC;
        REQUIRE(instruction.disassemble() == "ADC R7,R14,#3300012");

        alu->opcode = OpCode::SBC;
        REQUIRE(instruction.disassemble() == "SBC R7,R14,#3300012");

        alu->opcode = OpCode::RSC;
        REQUIRE(instruction.disassemble() == "RSC R7,R14,#3300012");

        alu->opcode = OpCode::ORR;
        REQUIRE(instruction.disassemble() == "ORR R7,R14,#3300012");

        alu->opcode = OpCode::MOV;
        REQUIRE(instruction.disassemble() == "MOV R7,#3300012");

        alu->opcode = OpCode::BIC;
        REQUIRE(instruction.disassemble() == "BIC R7,R14,#3300012");

        alu->opcode = OpCode::MVN;
        REQUIRE(instruction.disassemble() == "MVN R7,#3300012");
    }
}

TEST_CASE("Coprocessor Data Transfer", TAG) {
    uint32_t raw = 0b10101101101001011111000101000110;
    Instruction instruction(raw);
    CoprocessorDataTransfer* ldc = nullptr;

    REQUIRE((ldc = std::get_if<CoprocessorDataTransfer>(&instruction.data)));
    REQUIRE(instruction.condition == Condition::GE);

    REQUIRE(ldc->offset == 70);
    REQUIRE(ldc->cpn == 1);
    REQUIRE(ldc->crd == 15);
    REQUIRE(ldc->rn == 5);
    REQUIRE(ldc->load == false);
    REQUIRE(ldc->write == true);
    REQUIRE(ldc->len == false);
    REQUIRE(ldc->up == true);
    REQUIRE(ldc->pre == true);

    REQUIRE(instruction.disassemble() == "STCGE p1,c15,[R5,#70]!");

    ldc->load  = true;
    ldc->pre   = false;
    ldc->write = false;
    ldc->len   = true;

    REQUIRE(instruction.disassemble() == "LDCGEL p1,c15,[R5],#70");
}

TEST_CASE("Coprocessor Operand Operation", TAG) {
    uint32_t raw = 0b11101110101001011111000101000110;
    Instruction instruction(raw);
    CoprocessorDataOperation* cdp = nullptr;

    REQUIRE((cdp = std::get_if<CoprocessorDataOperation>(&instruction.data)));
    REQUIRE(instruction.condition == Condition::AL);

    REQUIRE(cdp->crm == 6);
    REQUIRE(cdp->cp == 2);
    REQUIRE(cdp->cpn == 1);
    REQUIRE(cdp->crd == 15);
    REQUIRE(cdp->crn == 5);
    REQUIRE(cdp->cp_opc == 10);

    REQUIRE(instruction.disassemble() == "CDP p1,10,c15,c5,c6,2");
}

TEST_CASE("Coprocessor Register Transfer", TAG) {
    uint32_t raw = 0b11101110101001011111000101010110;
    Instruction instruction(raw);
    CoprocessorRegisterTransfer* mrc = nullptr;

    REQUIRE(
      (mrc = std::get_if<CoprocessorRegisterTransfer>(&instruction.data)));
    REQUIRE(instruction.condition == Condition::AL);

    REQUIRE(mrc->crm == 6);
    REQUIRE(mrc->cp == 2);
    REQUIRE(mrc->cpn == 1);
    REQUIRE(mrc->rd == 15);
    REQUIRE(mrc->crn == 5);
    REQUIRE(mrc->load == false);
    REQUIRE(mrc->cp_opc == 5);

    REQUIRE(instruction.disassemble() == "MCR p1,5,R15,c5,c6,2");
}

TEST_CASE("Software Interrupt", TAG) {
    uint32_t raw = 0b00001111101010101010101010101010;
    Instruction instruction(raw);

    REQUIRE(instruction.condition == Condition::EQ);
    REQUIRE(instruction.disassemble() == "SWIEQ");
}

#undef TAG
