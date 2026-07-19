#include "CPU.h"

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