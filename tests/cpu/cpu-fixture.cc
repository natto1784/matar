#include "cpu-fixture.hh"

Psr
CpuFixture::psr(bool spsr) {
    uint32_t pc = getr(15);
    Psr psr(0);
    Cpu tmp = cpu;
    arm::Instruction instruction(
      Condition::AL,
      arm::PsrTransfer{ .operand = 0,
                        .spsr    = spsr,
                        .type    = arm::PsrTransfer::Type::Mrs,
                        .imm     = false });

    tmp.exec(instruction);

    psr.set_all(getr_(0, tmp));

    // reset pc
    setr(15, pc);
    return psr;
}

void
CpuFixture::set_psr(Psr psr, bool spsr) {
    uint32_t pc  = getr(15);
    uint32_t old = getr(0);
    setr(0, psr.raw());

    arm::Instruction instruction(
      Condition::AL,
      arm::PsrTransfer{ .operand = 0,
                        .spsr    = spsr,
                        .type    = arm::PsrTransfer::Type::Msr,
                        .imm     = false });

    cpu.exec(instruction);

    setr(0, old);

    // reset PC
    setr(15, pc);
}

// We need these workarounds to just use the public API and not private
// fields. Assuming that these work correctly is necessary. Besides, all that
// matters is that the public API is correct.
uint32_t
CpuFixture::getr_(uint8_t r, Cpu tmp) {
    uint32_t addr = 0x02000000;
    uint32_t word = bus->read_word(addr);
    uint32_t ret  = 0xFFFFFFFF;
    uint8_t base  = r ? 0 : 1;

    // set R0/R1 = addr
    arm::Instruction zero(
      Condition::AL,
      arm::DataProcessing{ .operand = addr,
                           .rd      = base,
                           .rn      = 0,
                           .set     = false,
                           .opcode  = arm::DataProcessing::OpCode::MOV });

    // get register
    arm::Instruction get(
      Condition::AL,
      arm::SingleDataTransfer{ .offset = static_cast<uint16_t>(0),
                               .rd     = r,
                               .rn     = base,
                               .load   = false,
                               .write  = false,
                               .byte   = false,
                               .up     = true,
                               .pre    = true });

    tmp.exec(zero);
    tmp.exec(get);

    ret = bus->read_word(addr);

    bus->write_word(addr, word);

    return ret - (r == 15 ? 4 : 0); // +4 for rd = 15 in str
}

void
CpuFixture::setr_(uint8_t r, uint32_t value, Cpu& cpu) {
    // set register
    arm::Instruction set(
      Condition::AL,
      arm::DataProcessing{
        .operand = (r == 15 ? value - 8 : value), // account for pipeline flush
        .rd      = r,
        .rn      = 0,
        .set     = false,
        .opcode  = arm::DataProcessing::OpCode::MOV });

    cpu.exec(set);
}
