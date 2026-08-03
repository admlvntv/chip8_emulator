#include "TerminalDisplay.h"
#include <iostream>
#include <string>

void TerminalDisplay::render() const {
    clear_terminal();

    std::string output;
    output.reserve((WIDTH + 1) * HEIGHT);

    for (int y{0}; y < HEIGHT; ++y) {
        for (int x{0}; x < WIDTH; ++x) {
            // Using a x character for pixels that are on, and space for off
            output += (m_pixels[y * WIDTH + x] ? "x" : " ");
        }
        output += '\n';
    }

    // Print current frame buffer
    std::cout << output << std::flush;
}

void TerminalDisplay::clear_terminal() {
    // ANSI escape code to clear screen and move cursor to (1,1)
    std::cout << "\033[2J\033[H" << std::flush;
}
