#include "CPU.h"

uint16_t CPU::fetch() {
  // Combine current and next byte
  uint16_t opcode = (m_memory.at(m_pc) << 8) | (m_memory.at(m_pc+1));
  m_pc += 2;
  return opcode;
}