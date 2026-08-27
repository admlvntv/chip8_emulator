#ifndef CHIP8_EMULATOR_SDLDISPLAY_H
#define CHIP8_EMULATOR_SDLDISPLAY_H

#include "Display.h"
#include <SDL3/SDL.h>

class SDLDisplay : public Display {
public:
    explicit SDLDisplay(int scale = 10); // No implicit conversion from int
    ~SDLDisplay() override;

    // Prevent copies
    SDLDisplay(const SDLDisplay&) = delete;
    SDLDisplay& operator=(const SDLDisplay&) = delete;

    void render() const override;

private:
    int m_scale;
    SDL_Window* m_window{nullptr};
    SDL_Renderer* m_renderer{nullptr};
};

#endif // CHIP8_EMULATOR_SDLDISPLAY_H
