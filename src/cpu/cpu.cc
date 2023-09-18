#include "cpu/cpu.hh"
#include "cpu-impl.hh"

Cpu::Cpu(const Bus& bus) noexcept
  : impl(std::make_unique<CpuImpl>(bus)){};

Cpu::~Cpu() = default;

void
Cpu::step() {
    impl->step();
};
