#include "Keypad.h"

void Keypad::press(uint8_t key) {
  if (key < KEY_COUNT) {
    m_keys[key] = true;
  }
}

void Keypad::release(uint8_t key) {
  if (key < KEY_COUNT) {
    m_keys[key] = false;
  }
}

bool Keypad::is_key_down(uint8_t key) const {
  return key < KEY_COUNT && m_keys[key];
}
