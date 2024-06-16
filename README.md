nothing to be seen here yet. LEAVE

But if you are curious (probably not), read ahead

# Dependencies
## Tested toolchains

- LLVM 18.1.7
- GCC 14.1.0

In theory, any toolchain supporting at least the c++23 standard should work.
I am using LLVM's clang and libcxx as the primary toolchain.

## Static libraries

| Name   | Version | Required? | Purpose   |
|:------:|:--------|:---------:|:---------:|
| catch2 | >= 3.4  | no        | for tests |

This goes without saying but using a different toolchain to compile these libraries before linking probably won't work.

# Status
- [x] CPU

  - [x] Arm
    - [x] Dissassembler
    - [x] Execution

  - [x] Thumb
    - [x] Dissassembler
    - [x] Execution
    
- [ ] Bus
  - [x] Cycle counting with CPU
  - [x] Reading memory
  - [x] Writing memory
  
- [ ] Scheduler (maybe?)
  - [ ] Sync PPU and CPU
  - [ ] Sync APU and CPU
  - [ ] Sync other stuff

- [ ] I/O
  - [ ] PPU
  - [ ] APU
  - [ ] Timers
  - [ ] DMA
  - [ ] Keypad

- Debugging
  - [x] GDB Remote Serial Protocol support
  
- Misc
  - [ ] Save/Load states
  - [x] Header Parsing

- Internal utilities
  - [x] Bit manipulation
  - [x] A global logger
  - [x] TCP Server (for GDB RSP)
  - [x] SHA256 hash (why? idk)

## Available unit tests so far
- CPU
  - Arm
    - Disassembler
    - Execution
  - Thumb
    - Disassembler
    - Execution
- Bus
  - Memory read/writes
  - Cycle Counting
- Some internal utility tests (idk why)
  

-----

# LOG
- June 11, 2024: After almost an year, I have come back to this silly abandoned project, will probably complete it soon.
- June 16, 2024: I ought to complete this soon
