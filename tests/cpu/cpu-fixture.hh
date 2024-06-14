#include "cpu/cpu.hh"

using namespace matar;

class CpuFixture {
  public:
    CpuFixture()
      : bus(std::shared_ptr<Bus>(
          new Bus(std::array<uint8_t, Bus::BIOS_SIZE>(),
                  std::vector<uint8_t>(Header::HEADER_SIZE))))
      , cpu(bus) {}

  protected:
    void exec(arm::InstructionData data, Condition condition = Condition::AL) {
        arm::Instruction instruction(condition, data);
        instruction.exec(cpu);
    }

    void exec(thumb::InstructionData data) {
        thumb::Instruction instruction(data);
        instruction.exec(cpu);
    }

    void reset(uint32_t value = 0) { setr(15, value + 8); }

    uint32_t getr(uint8_t r) { return getr_(r, cpu); }

    void setr(uint8_t r, uint32_t value) { setr_(r, value, cpu); }

    Psr psr(bool spsr = false);

    void set_psr(Psr psr, bool spsr = false);

    std::shared_ptr<Bus> bus;
    Cpu cpu;

  private:
    // hack to get a register
    uint32_t getr_(uint8_t r, Cpu& cpu);

    // hack to set a register
    void setr_(uint8_t r, uint32_t value, Cpu& cpu);
};
