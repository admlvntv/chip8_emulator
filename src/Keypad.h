#ifndef CHIP8_EMULATOR_KEYPAD_H
#define CHIP8_EMULATOR_KEYPAD_H

#include <array>
#include <cstddef>
#include <cstdint>

class Keypad {
public:
  static constexpr size_t KEY_COUNT{16}; // Hex keys 0x0-0xF

  void press(uint8_t key);
  void release(uint8_t key);
  bool is_key_down(uint8_t key) const;

private:
  std::array<bool, KEY_COUNT> m_keys{};
};

#endif // CHIP8_EMULATOR_KEYPAD_H
