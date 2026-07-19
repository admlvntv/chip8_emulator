#include "CPU.h"

namespace {
  uint8_t get_x(uint16_t opcode) {
    return (opcode & 0x0F00) >> 8;
  }

  uint8_t get_y(uint16_t opcode) {
    return (opcode & 0x00F0) >> 4;
  }

  uint8_t get_n(uint16_t opcode) {
    return opcode & 0x000F;
  }

  uint8_t get_nn(uint16_t opcode) {
    return opcode & 0x00FF;
  }

  uint16_t get_nnn(uint16_t opcode) {
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

CPU::Instruction CPU::decode(uint16_t opcode) const {
  const int id{ (opcode & 0xF000) >> 12 };
  const uint8_t x { get_x(opcode) };
  const uint8_t y { get_y(opcode) };
  const uint8_t n { get_n(opcode) };
  const uint8_t nn { get_nn(opcode) };
  const uint16_t nnn { get_nnn(opcode) };

  // Special cases
  if (id == 0x0) {
    return {(id << 8) | nn, 0xFF, 0xFF, nnn};
  }
  if (id == 0xE || id == 0xF) {
    return {(id << 8) | nn, x, 0xFF, 0xFF};
  }
  if (id == 0x8) {
    return {(id << 4) | n, x, y, 0xFF};
  }

  switch (id) {
  // NNN format
  case 0x1: case 0x2: case 0xA: case 0xB:
    return {id, 0xFF, 0xFF, nnn};

  // X, NN format
  case 0x3: case 0x4: case 0x6: case 0x7: case 0xC:
    return {id, x, 0xFF, nn};

  // X, Y format
  case 0x5: case 0x9:
    return {id, x, y, 0xFF};

  // X, Y, N format
  case 0xD:
    return {id, x, y, n};

  default:
    throw std::invalid_argument("Unknown opcode prefix");
  }
}