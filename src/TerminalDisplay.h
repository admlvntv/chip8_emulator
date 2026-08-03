#ifndef CHIP8_EMULATOR_TERMINALDISPLAY_H
#define CHIP8_EMULATOR_TERMINALDISPLAY_H

#include "Display.h"

class TerminalDisplay : public Display {
public:
    TerminalDisplay() = default;
    ~TerminalDisplay() override = default;

    void render() const override;

private:
    static void clear_terminal();
};

#endif // CHIP8_EMULATOR_TERMINALDISPLAY_H
