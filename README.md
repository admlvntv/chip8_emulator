# CHIP-8 Emulator

A from-scratch CHIP-8 interpreter written in C++, built to deepen my understanding of low-level computer architecture, memory management, and instruction set processing.

## Development Roadmap

- [x] **Phase 0: Architecture Research** (Manual decompilation and opcode mapping) [see analysis](research/pong_analysis.md)
- [x] **Phase 1: Terminal Foundation** (Core CPU, Memory, and Terminal-based display rendering)
- [ ] **Phase 2: Complete Opcode Set** (Implementing all 35 CHIP-8 opcodes, timers, and sound)
- [ ] **Phase 3: GUI & Input** (SDL/Graphics library integration and keypad support)
- [ ] **Phase 4: Optimization & Compatibility** (Super-CHIP support, quirks toggles, and performance tuning)

## System Design

The architecture and technical specifications of this emulator are documented in the [Design Document](docs/design.md).

## Build Instructions

This project uses CMake for out-of-source builds.

### Prerequisites
* A C++ compiler (supporting C++17 or later)
* CMake (3.10 or higher)

### Compiling
To build the emulator from the root directory:

```bash
mkdir build
cd build
cmake ..
make
```

### Running a ROM
```bash
./chip8_emulator <path_to_rom> [frequency_hz]
```
Example:
```bash
./chip8_emulator ../roms/1-chip8-logo.ch8 500
```

