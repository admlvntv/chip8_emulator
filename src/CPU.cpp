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


}