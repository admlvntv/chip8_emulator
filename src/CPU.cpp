#include "CPU.h"

uint16_t CPU::fetch() {
  // Combine current and next byte
  uint8_t high_byte{ m_memory.at(m_pc) };
  uint8_t low_byte{ m_memory.at(m_pc+1) };
  uint16_t opcode = { static_cast<uint16_t>((high_byte << 8) | low_byte) };
  m_pc += 2;
  return opcode;
}