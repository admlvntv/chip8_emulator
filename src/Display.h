#ifndef CHIP8_EMULATOR_DISPLAY_H
#define CHIP8_EMULATOR_DISPLAY_H

#include <array>

class Display {
public:
  static constexpr int WIDTH{64};
  static constexpr int HEIGHT{32};

  Display();
  virtual ~Display() = default;

  void clear();

  // Returns true if a pixel was flipped from 1 to 0 (collision flag for VF)
  bool write_pixel(int x, int y);

  uint8_t get_pixel(int x, int y) const;

  virtual void render() const = 0;

protected:
  std::array<uint8_t, WIDTH * HEIGHT> m_pixels{};
};

#endif // CHIP8_EMULATOR_DISPLAY_H