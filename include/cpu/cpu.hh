#include "bus.hh"

namespace matar {
class CpuImpl;

class Cpu {
  public:
    Cpu(const Bus& bus) noexcept;
    Cpu(const Cpu&)            = delete;
    Cpu(Cpu&&)                 = delete;
    Cpu& operator=(const Cpu&) = delete;
    Cpu& operator=(Cpu&&)      = delete;

    ~Cpu();

    void step();

  private:
    std::unique_ptr<CpuImpl> impl;
};
}
