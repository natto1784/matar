#include "cpu.hh"
#include "cpu/utility.hh"
#include "util/bits.hh"
#include "util/log.hh"
#include <cstdint>

using namespace logger;

std::string
Cpu::exec_arm(uint32_t insn) {
    Condition cond = static_cast<Condition>(get_bit_range(insn, 28, 31));
    std::string disassembled;

    auto pc_error = [](uint8_t r, const char* syn) {
        if (r == 15)
            log_error("Using PC (R15) as operand in {}", syn);
    };

    auto pc_undefined = [](uint8_t r, const char* syn) {
        if (r == 15)
            log_warn("Using PC (R15) as operand in {}", syn);
    };

    // Branch and exhcange
    if ((insn & 0x0FFFFFF0) == 0x012FFF10) {
        static constexpr char syn[] = "BX";

        uint8_t rn = insn & 0b1111;

        pc_undefined(rn, syn);

        disassembled = fmt::format("{}{} R{:d}", syn, cond, rn);

        if (cpsr.condition(cond)) {
            State state = static_cast<State>(rn & 1);

            // set state
            cpsr.set_state(state);

            // copy to PC
            gpr[15] = gpr[rn];

            // ignore [1:0] bits for arm and 0 bit for thumb
            rst_nth_bit(gpr[15], 0);
            if (state == State::Arm)
                rst_nth_bit(gpr[15], 1);
        }
        // Branch
    } else if ((insn & 0x0E000000) == 0x0A000000) {
        static constexpr char syn[] = "B";

        bool link       = get_nth_bit(insn, 24);
        uint32_t offset = get_bit_range(insn, 0, 23);

        disassembled =
          fmt::format("{}{}{} {:#08X}", syn, (link ? "L" : ""), cond, offset);

        if (cpsr.condition(cond)) {
            // lsh 2 and sign extend the 26 bit offset to 32
            // bits
            offset <<= 2;
            if (get_nth_bit(offset, 25))
                offset |= 0xFC000000;

            if (link)
                gpr[14] = gpr[15] - ARM_INSTRUCTION_SIZE;

            gpr[15] += offset;
        }

        // Multiply
    } else if ((insn & 0x0FC000F0) == 0x00000090) {
        static constexpr char syn[2][4] = { "MUL", "MLA" };

        uint8_t rm = get_bit_range(insn, 0, 3);
        uint8_t rs = get_bit_range(insn, 8, 11);
        uint8_t rn = get_bit_range(insn, 12, 15);
        uint8_t rd = get_bit_range(insn, 16, 19);
        bool s     = get_nth_bit(insn, 20);
        bool a     = get_nth_bit(insn, 21);

        if (rd == rm)
            log_error("rd and rm are not distinct in {} : {:d}", syn[a], rd);

        pc_error(rd, syn[a]);
        pc_error(rm, syn[a]);
        pc_error(rs, syn[a]);

        if (a) {
            pc_error(rn, syn[a]);
            disassembled = fmt::format("{}{}{} R{:d},R{:d},R{:d},R{:d}",
                                       syn[a],
                                       cond,
                                       (s ? "S" : ""),
                                       rd,
                                       rm,
                                       rs,
                                       rn);
        } else {
            disassembled = fmt::format("{}{}{} R{:d},R{:d},R{:d}",
                                       syn[a],
                                       cond,
                                       (s ? "S" : ""),
                                       rd,
                                       rm,
                                       rs);
        }

        if (cpsr.condition(cond)) {
            gpr[rd] = gpr[rm] * gpr[rs] + (a ? gpr[rn] : 0);

            if (s) {
                cpsr.set_z(!static_cast<bool>(gpr[rd]));
                cpsr.set_n(get_nth_bit(gpr[rd], 31));
                cpsr.set_c(0);
            }
        }
        // Multiply long
    } else if ((insn & 0x0F8000F0) == 0x00800090) {
        static constexpr char syn[2][2][6] = { { "SMULL", "SMLAL" },
                                               { "UMULL", "UMLAL" } };

        uint8_t rm   = get_bit_range(insn, 0, 3);
        uint8_t rs   = get_bit_range(insn, 8, 11);
        uint8_t rdlo = get_bit_range(insn, 12, 15);
        uint8_t rdhi = get_bit_range(insn, 16, 19);
        bool s       = get_nth_bit(insn, 20);
        bool a       = get_nth_bit(insn, 21);
        bool u       = get_nth_bit(insn, 22);

        if (rdhi == rdlo || rdhi == rm || rdlo == rm)
            log_error("rdhi, rdlo and rm are not distinct in {}", syn[u][a]);

        pc_error(rdhi, syn[u][a]);
        pc_error(rdlo, syn[u][a]);
        pc_error(rm, syn[u][a]);
        pc_error(rs, syn[u][a]);

        disassembled = fmt::format("{}{}{} R{:d},R{:d},R{:d},R{:d}",
                                   syn[u][a],
                                   cond,
                                   (s ? "S" : ""),
                                   rdlo,
                                   rdhi,
                                   rm,
                                   rs);

        if (cpsr.condition(cond)) {
            if (u) {
                uint64_t eval = static_cast<uint64_t>(gpr[rm]) *
                                  static_cast<uint64_t>(gpr[rs]) +
                                (a ? static_cast<uint64_t>(gpr[rdhi]) << 32 |
                                       static_cast<uint64_t>(gpr[rdlo])
                                   : 0);

                gpr[rdlo] = get_bit_range(eval, 0, 31);
                gpr[rdhi] = get_bit_range(eval, 32, 63);

            } else {
                int64_t eval = static_cast<uint64_t>(gpr[rm]) *
                                 static_cast<int64_t>(gpr[rs]) +
                               (a ? static_cast<int64_t>(gpr[rdhi]) << 32 |
                                      static_cast<int64_t>(gpr[rdlo])
                                  : 0);

                gpr[rdlo] = get_bit_range(eval, 0, 31);
                gpr[rdhi] = get_bit_range(eval, 32, 63);
            }

            if (s) {
                cpsr.set_z(!(static_cast<bool>(gpr[rdhi]) ||
                             static_cast<bool>(gpr[rdlo])));
                cpsr.set_n(get_nth_bit(gpr[rdhi], 31));
                cpsr.set_c(0);
                cpsr.set_v(0);
            }
        }

        // Single data swap
    } else if ((insn & 0x0FB00FF0) == 0x01000090) {
        static constexpr char syn[] = "SWP";

        uint8_t rm = get_bit_range(insn, 0, 3);
        uint8_t rd = get_bit_range(insn, 12, 15);
        uint8_t rn = get_bit_range(insn, 16, 19);
        bool b     = get_nth_bit(insn, 22);

        pc_undefined(rm, syn);
        pc_undefined(rn, syn);
        pc_undefined(rd, syn);

        disassembled = fmt::format(
          "{}{}{} R{:d},R{:d},[R{:d}]", syn, cond, (b ? "B" : ""), rd, rm, rn);

        if (cpsr.condition(cond)) {
            if (b) {
                gpr[rd] = bus->read_byte(gpr[rn]);
                bus->write_byte(gpr[rn], gpr[rm] & 0xFF);
            } else {
                gpr[rd] = bus->read_word(gpr[rn]);
                bus->write_word(gpr[rn], gpr[rm]);
            }
        }

        // Halfword transfer
        // TODO: create abstraction to reuse for block data and single data
        // transfer
    } else if ((insn & 0x0E000090) == 0x00000090) {
        static constexpr char syn[2][4] = { "STR", "LDR" };

        uint8_t rm = get_bit_range(insn, 0, 3);
        uint8_t h  = get_nth_bit(insn, 5);
        uint8_t s  = get_nth_bit(insn, 6);
        uint8_t rd = get_bit_range(insn, 12, 15);
        uint8_t rn = get_bit_range(insn, 16, 19);
        bool l     = get_nth_bit(insn, 20);
        bool w     = get_nth_bit(insn, 21);
        bool imm   = get_nth_bit(insn, 22);
        bool u     = get_nth_bit(insn, 23);
        bool p     = get_nth_bit(insn, 24);
        uint32_t offset;

        if (!p && w)
            log_error("Write-back enabled with post-indexing in {}", syn[l]);

        if (s && !l)
            log_error("Signed data found in {}", syn[l]);

        if (w)
            pc_error(rn, syn[l]);
        pc_error(rm, syn[l]);

        if (rd == 15 && !l && s && h)
            ;

        {
            offset = (imm ? get_bit_range(insn, 8, 11) << 4 | rm : gpr[rm]);
            std::string offset_str = fmt::format("{}{}{:d}",
                                                 (u ? "" : "-"),
                                                 (imm ? '#' : 'R'),
                                                 (imm ? offset : rm));

            disassembled = fmt::format(
              "{}{}{}{} R{:d}{}",
              syn[l],
              cond,
              (s ? "S" : ""),
              (h ? 'H' : 'B'),
              rd,
              (!offset ? fmt::format("[R{:d}]", rn)
               : p
                 ? fmt::format(",[R{:d},{}]{}", rn, offset_str, (w ? "!" : ""))
                 : fmt::format(",[R{:d}],{}", rn, offset_str)));
        }

        if (cpsr.condition(cond)) {
            uint32_t address = (u ? gpr[rn] + offset : gpr[rn] - offset);

            // load
            if (l) {
                // signed
                if (s) {
                    // halfword
                    if (h) {
                        gpr[rd] = bus->read_halfword(address);
                        // sign extend the halfword
                        if (get_nth_bit(gpr[rd], 15))
                            gpr[rd] |= 0xFFFF0000;
                        // byte
                    } else {
                        // sign extend the byte
                        gpr[rd] = bus->read_byte(address);
                        if (get_nth_bit(gpr[rd], 7))
                            gpr[rd] |= 0xFFFFFF00;
                    }
                    // unsigned halfword
                } else if (h) {
                    gpr[rd] = bus->read_halfword(address);
                }
                // store
            } else {
                // halfword
                if (h) {
                    // take PC into consideration
                    if (rd == 15)
                        address += 12;
                    bus->write_halfword(address, gpr[rd]);
                }
            }
        }
    }

    return disassembled;
}
