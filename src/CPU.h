#ifndef CHIP8_EMULATOR_CPU_H
#define CHIP8_EMULATOR_CPU_H

#include "display.h"
#include <array>
#include <string_view>

class CPU {
public:
  struct Instruction {
    int id{}; // Internal identifier for execution mapping (1st and sometimes last nibble)
    uint8_t reg_x{}; // VX register index (second nibble)
    uint8_t reg_y{}; // VY register index (third nibble)
    uint16_t input{}; // Holds immediate values: NNN (12-bit), NN (8-bit), or N (4-bit)
  };

  CPU();

  void load_rom(std::string_view filepath); // Loads ROM into memory at 0x200

  void cycle(Display& display); // Main CPU cycle
  void updateTimers(); // Decrements delay and sound timers. Call at 60Hz.

private:
  static constexpr size_t MEMORY_SIZE{4096};
  static constexpr size_t REGISTER_COUNT{16};
  static constexpr size_t STACK_DEPTH{12};
  static constexpr uint16_t ROM_START_ADDRESS{0x200};

  // CPU cycle
  uint16_t fetch(); // Returns raw big-endian 16-bit opcode and advances PC by 2
  Instruction decode(uint16_t opcode) const;
  void execute(const Instruction& instr, Display& display);

  // Initialize memory with fonts
  void load_fontset();

  // Hardware State
  std::array<uint8_t, MEMORY_SIZE> m_memory{};
  std::array<uint8_t, REGISTER_COUNT> m_V{}; // Registers V0 - VF
  uint16_t m_I{};

  uint16_t m_pc{ROM_START_ADDRESS};
  std::array<uint16_t, STACK_DEPTH> m_stack{};
  int m_sp{};

  uint8_t delay_timer{};
  uint8_t sound_timer{};
};

#endif // CHIP8_EMULATOR_CPU_H