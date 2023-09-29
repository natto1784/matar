#include "cpu-fixture.hh"

Psr
CpuFixture::psr(bool spsr) {
    Psr psr(0);
    CpuImpl tmp = cpu;
    arm::Instruction instruction(
      Condition::AL,
      arm::PsrTransfer{ .operand = 0,
                        .spsr    = spsr,
                        .type    = arm::PsrTransfer::Type::Mrs,
                        .imm     = false });

    instruction.exec(tmp);

    psr.set_all(getr_(0, tmp));
    return psr;
}

void
CpuFixture::set_psr(Psr psr, bool spsr) {
    // R0
    uint32_t old = getr(0);

    setr(0, psr.raw());

    arm::Instruction instruction(
      Condition::AL,
      arm::PsrTransfer{ .operand = 0,
                        .spsr    = spsr,
                        .type    = arm::PsrTransfer::Type::Msr,
                        .imm     = false });

    instruction.exec(cpu);

    setr(0, old);
}

// We need these workarounds to just use the public API and not private
// fields. Assuming that these work correctly is necessary. Besides, all that
// matters is that the public API is correct.
uint32_t
CpuFixture::getr_(uint8_t r, CpuImpl& cpu) {
    size_t addr   = 13000;
    size_t offset = r == 15 ? 4 : 0;
    uint32_t word = bus.read_word(addr + offset);
    CpuImpl tmp   = cpu;
    uint32_t ret  = 0xFFFFFFFF;
    uint8_t base  = r ? 0 : 1;

    // set R0/R1 = 0
    arm::Instruction zero(
      Condition::AL,
      arm::DataProcessing{ .operand = 0u,
                           .rd      = base,
                           .rn      = 0,
                           .set     = false,
                           .opcode  = arm::DataProcessing::OpCode::MOV });

    // get register
    arm::Instruction get(
      Condition::AL,
      arm::SingleDataTransfer{ .offset = static_cast<uint16_t>(addr),
                               .rd     = r,
                               .rn     = base,
                               .load   = false,
                               .write  = false,
                               .byte   = false,
                               .up     = true,
                               .pre    = true });

    zero.exec(tmp);
    get.exec(tmp);

    addr += offset;

    ret = bus.read_word(addr);

    bus.write_word(addr, word);

    return ret;
}

void
CpuFixture::setr_(uint8_t r, uint32_t value, CpuImpl& cpu) {
    // set register
    arm::Instruction set(
      Condition::AL,
      arm::DataProcessing{ .operand = value,
                           .rd      = r,
                           .rn      = 0,
                           .set     = false,
                           .opcode  = arm::DataProcessing::OpCode::MOV });

    set.exec(cpu);
}
