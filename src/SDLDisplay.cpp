#include "SDLDisplay.h"
#include <stdexcept>
#include <string>

SDLDisplay::SDLDisplay(int scale) : m_scale{scale} {
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        throw std::runtime_error(std::string("SDL_InitSubSystem failed: ") + SDL_GetError());
    }

    if (!SDL_CreateWindowAndRenderer("CHIP-8", WIDTH * m_scale, HEIGHT * m_scale, 0, &m_window, &m_renderer)) {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        throw std::runtime_error(std::string("SDL_CreateWindowAndRenderer failed: ") + SDL_GetError());
    }
}

SDLDisplay::~SDLDisplay() {
    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
    }
    if (m_window) {
        SDL_DestroyWindow(m_window);
    }
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void SDLDisplay::render() const {
    // Clear screen
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255); // black
    SDL_RenderClear(m_renderer);

    // Render each pixel as a square of side length m_scale
    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255); // white
    for (int y{0}; y < HEIGHT; ++y) {
        for (int x{0}; x < WIDTH; ++x) {
            if (m_pixels[y * WIDTH + x]) {
                const SDL_FRect rect{
                    static_cast<float>(x * m_scale),
                    static_cast<float>(y * m_scale),
                    static_cast<float>(m_scale),
                    static_cast<float>(m_scale)
                };
                SDL_RenderFillRect(m_renderer, &rect);
            }
        }
    }

    SDL_RenderPresent(m_renderer);
}
