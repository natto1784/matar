#include "io/timer/timer.hh"

namespace matar {
static constexpr auto TIMER_MAX = 0xFFFF;

/* lookup */
// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays)
static constexpr Task::Type tasks[NUM_TIMERS] = { Task::Type::TIMER0_OVERFLOW,
                                                  Task::Type::TIMER1_OVERFLOW,
                                                  Task::Type::TIMER2_OVERFLOW,
                                                  Task::Type::TIMER3_OVERFLOW };

static constexpr System::Irq irqs[NUM_TIMERS] = {
    System::Irq::TIMER0_OVERFLOW,
    System::Irq::TIMER1_OVERFLOW,
    System::Irq::TIMER2_OVERFLOW,
    System::Irq::TIMER3_OVERFLOW,
};

static constexpr uint32_t prescalers[4] = { 0, 6, 8, 10 };
// NOLINTEND(cppcoreguidelines-avoid-c-arrays)

void
Timer::schedule_overflow(uint8_t id, uint64_t at) {
    auto& timer = timers[id];
    auto ctrl   = timer.control;
    auto task   = tasks[id];

    auto cycles = (TIMER_MAX + 1 - timer.counter) << prescalers[ctrl.value.prescaler];
    scheduler.schedule_at(task, at + cycles);
}

void
Timer::write_and_eval_ctrl(uint8_t id, uint16_t raw) {
    auto& timer      = timers[id];
    auto& ctrl       = timer.control;
    bool was_enabled = ctrl.value.start_stop;

    ctrl.write(raw);

    if (ctrl.value.start_stop == true) {
        if (was_enabled == false) {
            timer.counter = timer.reload;
        }

        if (!ctrl.value.count_up) {
            schedule_overflow(id, scheduler.get_cycles());
        }
    } else {
    }
};

void
Timer::trigger_overflow(uint8_t id, uint64_t at) {
    auto& timer = timers[id];
    auto ctrl  = timer.control;

    if (!timer.control.value.start_stop) {
        return;
    }

    timer.counter = timer.reload;

    if (ctrl.value.irq_enable) {
        irq.raise_irq(irqs[id]);
    }

    if (id != 3) {
        auto& next = timers[id + 1];
        if (next.control.value.count_up) {
            if (next.counter == TIMER_MAX) {
                trigger_overflow(id + 1, at);
            } else {
                next.counter++;
            }
        }
    }

    if (id <= 1) {
        sound.dma_playback(id, at);
    }

    schedule_overflow(id, at);
}
}
