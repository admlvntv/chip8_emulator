#ifndef CHIP8_EMULATOR_CPU_H
#define CHIP8_EMULATOR_CPU_H

#include "Display.h"
#include <array>
#include <stack>
#include <cstdint>
#include <string>

class CPU {
public:
  CPU();

  void load_rom(const std::string& filepath); // Loads ROM into memory at 0x200

  void cycle(Display& display); // Main CPU cycle
  void updateTimers(); // Decrements delay and sound timers. Call at 60Hz.

private:
  static constexpr size_t MEMORY_SIZE{4096};
  static constexpr size_t REGISTER_COUNT{16};
  static constexpr size_t STACK_DEPTH{12};
  static constexpr uint16_t ROM_START_ADDRESS{0x200};
  static constexpr uint16_t FONT_START_ADDRESS{0x50};
  static constexpr uint8_t FONT_CHAR_SIZE{5}; // Bytes per hex digit sprite

  // CPU cycle
  uint16_t fetch(); // Returns raw big-endian 16-bit opcode and advances PC by 2
  void execute(uint16_t opcode, Display &display);

  // Initialize memory with fonts
  void load_fontset();

  // Hardware State
  std::array<uint8_t, MEMORY_SIZE> m_memory{};
  std::array<uint8_t, REGISTER_COUNT> m_V{}; // Registers V0 - VF
  uint16_t m_I{};

  uint16_t m_pc{ROM_START_ADDRESS};
  std::stack<uint16_t> m_stack{};

  uint8_t m_delay_timer{};
  uint8_t m_sound_timer{};

  friend class CPUForTesting;
};

#endif // CHIP8_EMULATOR_CPU_H