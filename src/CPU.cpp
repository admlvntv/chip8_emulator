#include "CPU.h"

namespace {
  constexpr uint8_t get_x(uint16_t opcode) {
    return (opcode & 0x0F00) >> 8;
  }

  constexpr uint8_t get_y(uint16_t opcode) {
    return (opcode & 0x00F0) >> 4;
  }

  constexpr uint8_t get_n(uint16_t opcode) {
    return opcode & 0x000F;
  }

  constexpr uint8_t get_nn(uint16_t opcode) {
    return opcode & 0x00FF;
  }

  constexpr uint16_t get_nnn(uint16_t opcode) {
    return opcode & 0x0FFF;
  }
}

uint16_t CPU::fetch() {
  // Out of bounds checking
  if (m_pc >= MEMORY_SIZE - 1) {
    throw std::out_of_range("PC out of bounds");
  }

  // Combine current and next byte
  uint8_t high_byte{ m_memory[m_pc] };
  uint8_t low_byte{ m_memory[m_pc+1] };
  uint16_t opcode = { static_cast<uint16_t>((high_byte << 8) | low_byte) };
  m_pc += 2;
  return opcode;
}

void CPU::execute(uint16_t opcode, Display &display) {
  const int id{ (opcode & 0xF000) >> 12 };
  const uint8_t x { get_x(opcode) };
  const uint8_t y { get_y(opcode) };
  const uint8_t n { get_n(opcode) };
  const uint8_t nn { get_nn(opcode) };
  const uint16_t nnn { get_nnn(opcode) };

  switch (id) {
  case 0x0: {
    switch (nn) {
    case 0xE0:
      display.clear();
      break;
    case 0xEE:
      // TODO
      break;
    default:
      throw std::invalid_argument("Unknown 0x0 opcode");
    }
    break;
  }

  case 0x1:
    if (nnn >= ROM_START_ADDRESS && nnn <= MEMORY_SIZE) {
      m_pc = nnn;
    }
    else {
      throw std::invalid_argument("Invalid GOTO address");
    }
    break;

  case 0x6:
    m_V[x] = nn;
    break;

  case 0x7:
    m_V[x] += nn;
    break;

  case 0xA:
    m_I = nnn;
    break;

  case 0xD: {
    uint8_t sprite_height{n};
    m_V[0xF] = 0; // Reset collision flag

    // Wrap start position
    int start_x{ m_V[x] % Display::WIDTH };
    int start_y{ m_V[y] % Display::HEIGHT };

    for (int row{0}; row < sprite_height; row++) {
      int screen_y {start_y + row};
      // Stop drawing rows past the bottom edge
      if (screen_y >= Display::HEIGHT) break;

      uint8_t pixel_data{m_memory[m_I + row]};

      for (int bit_pos{7}; bit_pos >= 0; --bit_pos) {
        // Check bit from left to right (7 down to 0)
        if ((pixel_data & (1 << bit_pos)) != 0) {
          int screen_x{ start_x + (7 - bit_pos) };
          // Skip pixels past the right edge
          if (screen_x >= Display::WIDTH) continue;

          // Write pixel (returns true if an existing pixel was turned OFF)
          if (display.write_pixel(screen_x, screen_y)) {
            m_V[0xF] = 1;
          }
        }
      }
    }
    break;
  }

  default:
    throw std::invalid_argument("Unknown opcode prefix");
  }
}