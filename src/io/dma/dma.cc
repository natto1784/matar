#include "bus.hh"
#include "scheduler.hh"
#include "util/log.hh"
#include <algorithm>

namespace matar {
/* lookup */
// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays)

static constexpr Task::Type tasks[NUM_DMA_CHANS] = {
    Task::Type::DMA0_ACTIVATE,
    Task::Type::DMA1_ACTIVATE,
    Task::Type::DMA2_ACTIVATE,
    Task::Type::DMA3_ACTIVATE,
};

static constexpr System::Irq irqs[NUM_DMA_CHANS] = {
    System::Irq::DMA0,
    System::Irq::DMA1,
    System::Irq::DMA2,
    System::Irq::DMA3,
};

// NOLINTEND(cppcoreguidelines-avoid-c-arrays)

void
Dma::write_and_eval_ctrl(uint8_t id, uint16_t raw) {
    auto& chan       = channels[id];
    auto& ctrl       = chan.control;
    bool was_enabled = ctrl.value.enable;

    ctrl.write(raw);

    if (ctrl.value.enable == true) {
        chan.timestamp = scheduler.get_cycles();

        if (was_enabled) {
            if (ctrl.value.timing ==
                static_cast<uint8_t>(DmaControl::Timing::Immediately)) {
                scheduler.schedule_from_now(tasks[id], 3);
            }
        }
    }
};

void
Dma::start_transfer(uint8_t id) {
    auto& chan = channels[id];
    int ws     = chan.control.value.transfer_32 ? 4 : 2;

    uint32_t src = chan.source;
    uint32_t dst = chan.destination;
    int step_src = 0;
    int step_dst = 0;

    CpuAccess access = CpuAccess::NonSequential;

    if (!chan.control.value.enable)
        return;

    switch (chan.control.value.src_ctrl) {
        case 0: {
            /* Increment */
            step_src = ws;
            break;
        }
        case 1: {
            /* Decrement */
            step_src = -ws;
            break;
        }
        case 2: {
            /* Fixed */
            step_src = 0;
            break;
        }
        default: {
            glogger.error("this is NOT supposed to happen");
            std::abort();
        }
    }

    switch (chan.control.value.dst_ctrl) {
        case 0:
        case 3: {
            /* Increment */
            step_dst = ws;
            break;
        }
        case 1: {
            /* Decrement */
            step_dst = -ws;
            break;
        }
        case 2: {
            /* Fixed */
            step_dst = 0;
            break;
        }
        default: {
            glogger.error("this is NOT supposed to happen");
            std::abort();
        }
    }

    /* Sound FIFOs */
    if ((chan.control.value.timing ==
         static_cast<uint8_t>(DmaControl::Timing::Special)) &&
        (chan.destination == sound::FIFO_A ||
         chan.destination == sound::FIFO_B) &&
        (id == 1 || id == 2) && chan.control.value.repeat) {
        for (int i = 0; i < 4; i++) {
            uint32_t word = bus.read_word(src, access);
            bus.write_word(dst, word, access);
            access = CpuAccess::Sequential;
            src += 4;
        }
    } else if (ws == 4) {
        for (int i = 0; i < chan.word_count; i++) {
            uint32_t word = bus.read_word(src, access);
            bus.write_word(dst, word, access);
            access = CpuAccess::Sequential;
            src += step_src;
            dst += step_dst;
        }
    } else {
        for (int i = 0; i < chan.word_count; i++) {
            uint16_t word = bus.read_halfword(src, access);
            bus.write_halfword(dst, word, access);
            access = CpuAccess::Sequential;
            src += step_src;
            dst += step_dst;
        }
    }

    if (chan.control.value.irq_enable) {
        system.raise_irq(irqs[id]);
    }

    if (!chan.control.value.repeat) {
        chan.control.value.enable = false;
    }
}

void
Dma::notify(DmaControl::Timing timing, uint64_t at) {
    for (int id = 0; id < NUM_DMA_CHANS; id++) {
        auto& chan = channels[id];

        if (chan.control.value.enable &&
            chan.control.value.timing == static_cast<uint8_t>(timing)) {
            uint64_t cycles = 3 - std::min<uint64_t>(
                                    scheduler.get_cycles() - chan.timestamp, 3);

            scheduler.schedule_at(tasks[id], cycles + at);
        }
    }
}

void
Dma::schedule_sound_xfer(uint32_t fifo_addr, uint64_t at) {
    for (int id = 1; id <= 2; id++) {
        auto& chan = channels[id];

        if (chan.control.value.enable &&
            (chan.control.value.timing ==
             static_cast<uint8_t>(DmaControl::Timing::Special)) &&
            (chan.destination == fifo_addr)) {
            uint64_t cycles = 3 - std::min<uint64_t>(
                                    scheduler.get_cycles() - chan.timestamp, 3);

            scheduler.schedule_at(tasks[id], cycles + at);
        }
    }
}
}
