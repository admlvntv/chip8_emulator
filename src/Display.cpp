#include "Display.h"

Display::Display() {
    clear();
}

void Display::clear() {
    m_pixels.fill(0);
}

bool Display::write_pixel(int x, int y) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
        return false;
    }

    // XOR operation
    int index{ y * WIDTH + x };
    uint8_t old_val{m_pixels[index]};
    m_pixels[index] ^= 1;

    // Return true if pixel was erased (1 -> 0)
    return (old_val == 1 && m_pixels[index] == 0);
}

uint8_t Display::get_pixel(int x, int y) const {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
        return 0;
    }
    return m_pixels[y * WIDTH + x];
}
