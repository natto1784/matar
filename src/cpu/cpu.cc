#include "cpu/cpu.hh"
#include "cpu-impl.hh"

namespace matar {
Cpu::Cpu(const Bus& bus) noexcept
  : impl(std::make_unique<CpuImpl>(bus)){};

Cpu::~Cpu() = default;

void
Cpu::step() {
    impl->step();
};
}
