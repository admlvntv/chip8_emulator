#include "CPU.h"

#include <fstream>
#include <random>
#include <stdexcept>

CPU::CPU() {
  load_fontset();
}

void CPU::load_rom(const std::string& filepath) {
  // Open at end so tellg() returns ROM size immediately
  std::ifstream rom_file(filepath, std::ios::binary | std::ios::ate);
  if (!rom_file.is_open()) {
    throw std::runtime_error("Failed to open ROM file");
  }

  const std::streamsize rom_size{rom_file.tellg()};
  if (rom_size < 0) {
    throw std::runtime_error("Failed to read ROM size");
  }

  // ROM data is loaded at 0x200, so only memory from 0x200-0xFFF is available
  constexpr size_t max_rom_size{MEMORY_SIZE - ROM_START_ADDRESS};
  if (static_cast<size_t>(rom_size) > max_rom_size) {
    throw std::runtime_error("ROM is too large to fit in memory");
  }

  // Rewind and copy raw bytes into memory
  rom_file.seekg(0, std::ios::beg);
  if (!rom_file.read(reinterpret_cast<char*>(m_memory.data() + ROM_START_ADDRESS), rom_size)) {
    throw std::runtime_error("Failed to read ROM file");
  }
}

void CPU::load_fontset() {
  static constexpr std::array<uint8_t, 80> fontset {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
  };

  // Store fontset in memory starting at FONT_START_ADDRESS
  for (size_t i{0}; i < fontset.size(); ++i) {
    m_memory[FONT_START_ADDRESS + i] = fontset[i];
  }

}

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
  uint16_t opcode{ static_cast<uint16_t>((high_byte << 8) | low_byte) };
  m_pc += 2;
  return opcode;
}

void CPU::execute(uint16_t opcode, Display &display, const Keypad &keypad) {
  const int id{ (opcode & 0xF000) >> 12 };
  const uint8_t x{ get_x(opcode) };
  const uint8_t y{ get_y(opcode) };
  const uint8_t n{ get_n(opcode) };
  const uint8_t nn{ get_nn(opcode) };
  const uint16_t nnn{ get_nnn(opcode) };

  switch (id) {
  case 0x0: {
    switch (nnn) {
    case 0x0E0:
      display.clear();
      break;
    case 0x0EE:
      if (!m_stack.empty()) {
        m_pc = m_stack.top();
        m_stack.pop();
      } else {
        throw std::out_of_range("Cannot pop an empty stack");
      }
      break;
    default:
      throw std::invalid_argument("Unknown 0x0 opcode");
    }
    break;
  }

  case 0x1:
    if (nnn >= ROM_START_ADDRESS && nnn < MEMORY_SIZE) {
      m_pc = nnn;
    }
    else {
      throw std::invalid_argument("Invalid GOTO address");
    }
    break;

  case 0x2:
    if (nnn >= ROM_START_ADDRESS && nnn < MEMORY_SIZE) {
      if (m_stack.size() >= STACK_DEPTH) {
        throw std::overflow_error("Stack overflow");
      }
      m_stack.push(m_pc);
      m_pc = nnn;
    }
    else {
      throw std::invalid_argument("Invalid GOTO address");
    }
    break;

  case 0x3:
    if (m_V[x] == nn) {
      m_pc += 2;
    }
    break;

  case 0x4:
    if (m_V[x] != nn) {
      m_pc += 2;
    }
    break;

  case 0x5:
    if (n != 0x0) {
      throw std::invalid_argument("Invalid sub opcode for 0x5");
    }
    if (m_V[x] == m_V[y]) {
      m_pc += 2;
    }
    break;

  case 0x6:
    m_V[x] = nn;
    break;

  case 0x7:
    m_V[x] += nn;
    break;

  case 0x8:
    switch (n) {
    case 0x0:
      m_V[x] = m_V[y];
      break;

    case 0x1:
      m_V[x] = m_V[x] | m_V[y];
      break;

    case 0x2:
      m_V[x] = m_V[x] & m_V[y];
      break;

    case 0x3:
      m_V[x] = m_V[x] ^ m_V[y];
      break;

    case 0x4: {
      // Compute the flag in advance
      // VF still ends up holding the carry flag even if X or Y is VF
      uint8_t vx{m_V[x]};
      uint8_t vy{m_V[y]};
      uint8_t carry{ static_cast<uint8_t>(vy > UINT8_MAX - vx ? 1 : 0) }; // Set VF flag if VX+VY > 255
      m_V[x] = vx + vy;
      m_V[0xF] = carry;
      break;
    }

    case 0x5: {
      // Set VF flag if minuend (first operand) >= subtrahend (second operand)
      // Compute the flag in advance
      // VF still ends up holding the carry flag even if X or Y is VF
      uint8_t vx{m_V[x]};
      uint8_t vy{m_V[y]};
      uint8_t no_borrow{ static_cast<uint8_t>(vx >= vy ? 1 : 0) };
      m_V[x] = vx - vy;
      m_V[0xF] = no_borrow;
      break;
    }

    case 0x6: {
      // TODO: add quirk for SUPER-CHIP
      // Capture VY before writing VX (in case X is VF) and write the flag last (in case Y is VF)
      uint8_t vy{m_V[y]};
      uint8_t shifted_out{ static_cast<uint8_t>(vy & 0b00000001) };
      m_V[x] = vy >> 1;
      m_V[0xF] = shifted_out;
      break;
    }
    case 0x7: {
      // Set VF flag if minuend (first operand) >= subtrahend (second operand)
      // Compute the flag in advance and write it last, so VF still ends up holding the borrow flag even if X or Y is VF
      uint8_t vx{m_V[x]};
      uint8_t vy{m_V[y]};
      uint8_t no_borrow{ static_cast<uint8_t>(vy >= vx ? 1 : 0) };
      m_V[x] = vy - vx;
      m_V[0xF] = no_borrow;
      break;
    }

    case 0xE: {
      // TODO: add quirk for SUPER-CHIP
      // Capture VY before writing VX (in case X is VF) and write the flag last (in case Y is VF)
      uint8_t vy{m_V[y]};
      uint8_t shifted_out{ static_cast<uint8_t>((vy & 0b10000000) >> 7) };
      m_V[x] = vy << 1;
      m_V[0xF] = shifted_out;
      break;
    }

    default:
      throw std::invalid_argument("Unknown 0x8 opcode");
    }
    break;

  case 0x9:
    if (n != 0x0) {
      throw std::invalid_argument("Invalid sub opcode for 0x9");
    }
    if (m_V[x] != m_V[y]) {
      m_pc += 2;
    }
    break;

  case 0xA:
    m_I = nnn;
    break;

  case 0xB: {
    // TODO: add quirk for SUPER-CHIP (BXNN)
    uint16_t jump_address{ static_cast<uint16_t>(nnn + m_V[0x0]) };
    if (jump_address >= ROM_START_ADDRESS && jump_address < MEMORY_SIZE) {
      m_pc = jump_address;
    }
    else {
      throw std::invalid_argument("Invalid GOTO address");
    }
    break;
  }

  case 0xC: {
    static std::mt19937 rng{ std::random_device{}() };
    static std::uniform_int_distribution<uint8_t> byte_dist(0, UINT8_MAX);
    m_V[x] = byte_dist(rng) & nn;
    break;
  }

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

  case 0xF:
    switch (nn) {
    case 0x07:
      m_V[x] = m_delay_timer;
      break;

    case 0x15:
      m_delay_timer = m_V[x];
      break;

    case 0x1E:
      m_I += m_V[x];
      break;

    case 0x29:
      // Point I at the font sprite for the hex digit in VX's low nibble (digits 0-F only)
      m_I = FONT_START_ADDRESS + (m_V[x] & 0x0F) * FONT_CHAR_SIZE;
      break;

    case 0x33: {
      uint8_t value{m_V[x]};
      m_memory[m_I + 2] = value % 10; // ones place
      value /= 10;
      m_memory[m_I + 1] = value % 10; // tens place
      value /= 10;
      m_memory[m_I] = value % 10; // hundreds place
      break;
    }

    case 0x55:
      // TODO: add quirk for SUPER-CHIP
      for (uint8_t i{0}; i <= x; ++i) {
        m_memory[m_I + i] = m_V[i];
      }
      m_I += x + 1;
      break;

    case 0x65:
      // TODO: add quirk for SUPER-CHIP
      for (uint8_t i{0}; i <= x; ++i) {
        m_V[i] = m_memory[m_I + i];
      }
      m_I += x + 1;
      break;

    default:
      throw std::invalid_argument("Unknown 0xF opcode");
    }
    break;

  default:
    throw std::invalid_argument("Unknown opcode prefix");
  }
}

void CPU::cycle(Display &display, const Keypad &keypad) {
  execute(fetch(), display, keypad);
}

void CPU::updateTimers() {
  if (m_delay_timer > 0) {
    m_delay_timer--;
  }
  // TODO: implement sound timer
}