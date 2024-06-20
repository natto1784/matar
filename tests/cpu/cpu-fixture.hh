#include "cpu/cpu.hh"

using namespace matar;

class CpuFixture {
  public:
    CpuFixture()
      : bus(Bus::init(std::array<uint8_t, Bus::BIOS_SIZE>(),
                      std::vector<uint8_t>(Header::HEADER_SIZE)))
      , cpu(bus) {}

  protected:
    void exec(arm::InstructionData data, Condition condition = Condition::AL) {
        // hack to account for one fetch cycle
        bus->internal_cycle();

        arm::Instruction instruction(condition, data);
        cpu.exec(instruction);
    }

    void exec(thumb::InstructionData data) {
        // hack to account for one fetch cycle
        bus->internal_cycle();

        thumb::Instruction instruction(data);
        cpu.exec(instruction);
    }

    void reset(uint32_t value = 0) { setr(15, value + 8); }

    uint32_t getr(uint8_t r) {
        uint32_t pc = 0;

        if (r != 15)
            pc = getr_(15, cpu);

        uint32_t ret = getr_(r, cpu);

        if (r == 15)
            pc = ret;

        // undo PC advance
        setr_(15, pc, cpu);

        return ret;
    }

    void setr(uint8_t r, uint32_t value) {
        uint32_t pc = getr_(15, cpu);
        setr_(r, value, cpu);

        // undo PC advance when r != 15
        // when r is 15, setr_ takes account of pipeline flush
        if (r != 15)
            setr_(15, pc, cpu);
    }

    Psr psr(bool spsr = false);

    void set_psr(Psr psr, bool spsr = false);

    std::shared_ptr<Bus> bus;
    Cpu cpu;

  private:
    // hack to get a register
    uint32_t getr_(uint8_t r, Cpu tmp);

    // hack to set a register
    void setr_(uint8_t r, uint32_t value, Cpu& cpu);
};
