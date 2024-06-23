#pragma once

#include "../../src/util/log.hh"
#include <cstdint>
#include <queue>

namespace matar {

struct Task {
    enum class Type {
        DMA0_ACTIVATE,
        DMA1_ACTIVATE,
        DMA2_ACTIVATE,
        DMA3_ACTIVATE,
        TIMER0_OVERFLOW,
        TIMER1_OVERFLOW,
        TIMER2_OVERFLOW,
        TIMER3_OVERFLOW,
        DISPLAY_HDRAW,
        DISPLAY_HBLANK,
        SAMPLE_PWM,
    };

    Type type;
    uint64_t cycles;

    bool operator<(const Task& other) const { return cycles > other.cycles; }
};

class Scheduler {
  public:
    void schedule_at(Task::Type type, uint64_t cycles) {
        tasks.push({ type, cycles });
    }

    void schedule_from_now(Task::Type type, uint64_t cycles) {
        schedule_at(type, this->cycles + cycles);
    }

    uint64_t get_cycles() const { return cycles; }

    void add_cycles(uint64_t cycles) { this->cycles += cycles; }

    bool empty() const { return tasks.empty(); }

    Task top() const { return tasks.top(); }

    void pop() { tasks.pop(); }

  private:
    std::priority_queue<Task> tasks;
    uint64_t cycles = 0;
};
}
