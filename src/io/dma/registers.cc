#include "io/dma/dma.hh"
#include "util/log.hh"

namespace matar {
static constexpr uint32_t DMA0SAD   = 0x40000B0;
static constexpr uint32_t DMA0DAD   = 0x40000B4;
static constexpr uint32_t DMA0CNT_L = 0x40000B8;
static constexpr uint32_t DMA0CNT_H = 0x40000BA;
static constexpr uint32_t DMA1SAD   = 0x40000BC;
static constexpr uint32_t DMA1DAD   = 0x40000C0;
static constexpr uint32_t DMA1CNT_L = 0x40000C4;
static constexpr uint32_t DMA1CNT_H = 0x40000C6;
static constexpr uint32_t DMA2SAD   = 0x40000C8;
static constexpr uint32_t DMA2DAD   = 0x40000CC;
static constexpr uint32_t DMA2CNT_L = 0x40000D0;
static constexpr uint32_t DMA2CNT_H = 0x40000D2;
static constexpr uint32_t DMA3SAD   = 0x40000D4;
static constexpr uint32_t DMA3DAD   = 0x40000D8;
static constexpr uint32_t DMA3CNT_L = 0x40000DC;
static constexpr uint32_t DMA3CNT_H = 0x40000DE;

uint16_t
Dma::read_halfword(uint32_t address) const {

    switch (address) {
        case DMA0CNT_H: {
            return channels[0].control.read();
        }
        case DMA1CNT_H: {
            return channels[1].control.read();
        }
        case DMA2CNT_H: {
            return channels[2].control.read();
        }
        case DMA3CNT_H: {
            return channels[3].control.read();
        }
        case DMA0SAD: {
            return channels[0].source & 0xFFFF;
        }
        case DMA0SAD + 2: {
            return channels[0].source >> 16;
        }
        case DMA0DAD: {
            return channels[0].destination & 0xFFFF;
        }
        case DMA0DAD + 2: {
            return channels[0].destination >> 16;
        }
        case DMA0CNT_L: {
            return channels[0].word_count;
        }
        case DMA1SAD: {
            return channels[1].source & 0xFFFF;
        }
        case DMA1SAD + 2: {
            return channels[1].source >> 16;
        }
        case DMA1DAD: {
            return channels[1].destination & 0xFFFF;
        }
        case DMA1DAD + 2: {
            return channels[1].destination >> 16;
        }
        case DMA1CNT_L: {
            return channels[1].word_count;
        }
        case DMA2SAD: {
            return channels[2].source & 0xFFFF;
        }
        case DMA2SAD + 2: {
            return channels[2].source >> 16;
        }
        case DMA2DAD: {
            return channels[2].destination & 0xFFFF;
        }
        case DMA2DAD + 2: {
            return channels[2].destination >> 16;
        }
        case DMA2CNT_L: {
            return channels[2].word_count;
        }
        case DMA3SAD: {
            return channels[3].source & 0xFFFF;
        }
        case DMA3SAD + 2: {
            return channels[3].source >> 16;
        }
        case DMA3DAD: {
            return channels[3].destination & 0xFFFF;
        }
        case DMA3DAD + 2: {
            return channels[3].destination >> 16;
        }
        case DMA3CNT_L: {
            return channels[3].word_count;
        }
        default: {
            glogger.warn("Invalid DMA I/O address read at 0x{:08X}", address);
        }
    }

    return 0xFFFF;
}

void
Dma::write_halfword(uint32_t address, uint16_t halfword) {
    switch (address) {
        case DMA0CNT_H: {
            write_and_eval_ctrl(0, halfword);
            break;
        }
        case DMA1CNT_H: {
            write_and_eval_ctrl(1, halfword);
            break;
        }
        case DMA2CNT_H: {
            write_and_eval_ctrl(2, halfword);
            break;
        }
        case DMA3CNT_H: {
            write_and_eval_ctrl(3, halfword);
            break;
        }
        case DMA0SAD: {
            channels[0].source &= ~0xFFFF;
            channels[0].source |= halfword;
            break;
        }
        case DMA0SAD + 2: {
            channels[0].source &= ~0xFFFF0000;
            channels[0].source |= (halfword << 16);
            break;
        }
        case DMA0DAD: {
            channels[0].destination &= ~0xFFFF;
            channels[0].destination |= halfword;
            break;
        }
        case DMA0DAD + 2: {
            channels[0].destination &= ~0xFFFF0000;
            channels[0].destination |= (halfword << 16);
            break;
        }
        case DMA0CNT_L: {
            channels[0].word_count = halfword;
            break;
        }
        case DMA1SAD: {
            channels[1].source &= ~0xFFFF;
            channels[1].source |= halfword;
            break;
        }
        case DMA1SAD + 2: {
            channels[1].source &= ~0xFFFF0000;
            channels[1].source |= (halfword << 16);
            break;
        }
        case DMA1DAD: {
            channels[1].destination &= ~0xFFFF;
            channels[1].destination |= halfword;
            break;
        }
        case DMA1DAD + 2: {
            channels[1].destination &= ~0xFFFF0000;
            channels[1].destination |= (halfword << 16);
            break;
        }
        case DMA1CNT_L: {
            channels[1].word_count = halfword;
            break;
        }
        case DMA2SAD: {
            channels[2].source &= ~0xFFFF;
            channels[2].source |= halfword;
            break;
        }
        case DMA2SAD + 2: {
            channels[2].source &= ~0xFFFF0000;
            channels[2].source |= (halfword << 16);
            break;
        }
        case DMA2DAD: {
            channels[2].destination &= ~0xFFFF;
            channels[2].destination |= halfword;
            break;
        }
        case DMA2DAD + 2: {
            channels[2].destination &= ~0xFFFF0000;
            channels[2].destination |= (halfword << 16);
            break;
        }
        case DMA2CNT_L: {
            channels[2].word_count = halfword;
            break;
        }
        case DMA3SAD: {
            channels[3].source &= ~0xFFFF;
            channels[3].source |= halfword;
            break;
        }
        case DMA3SAD + 2: {
            channels[3].source &= ~0xFFFF0000;
            channels[3].source |= (halfword << 16);
            break;
        }
        case DMA3DAD: {
            channels[3].destination &= ~0xFFFF;
            channels[3].destination |= halfword;
            break;
        }
        case DMA3DAD + 2: {
            channels[3].destination &= ~0xFFFF0000;
            channels[3].destination |= (halfword << 16);
            break;
        }
        case DMA3CNT_L: {
            channels[3].word_count = halfword;
            break;
        }
        default: {
            glogger.warn("Unused sound IO address written at 0x{:08X}",
                         address);
        }
    }
}
}
