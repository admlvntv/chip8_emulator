#include "CPU.h"
#include "SDLDisplay.h"

#include <SDL3/SDL.h>
#include <chrono>
#include <iostream>
#include <optional>
#include <thread>

namespace {

// COSMAC VIP keypad remapped to a modern keyboard:
/*
  1 2 3 C    1 2 3 4
  4 5 6 D -> Q W E R
  7 8 9 E    A S D F
  A 0 B F    Z X C V
*/
std::optional<uint8_t> scancode_to_key(SDL_Scancode scancode) {
  switch (scancode) {
    case SDL_SCANCODE_1: return 0x1;
    case SDL_SCANCODE_2: return 0x2;
    case SDL_SCANCODE_3: return 0x3;
    case SDL_SCANCODE_4: return 0xC;
    case SDL_SCANCODE_Q: return 0x4;
    case SDL_SCANCODE_W: return 0x5;
    case SDL_SCANCODE_E: return 0x6;
    case SDL_SCANCODE_R: return 0xD;
    case SDL_SCANCODE_A: return 0x7;
    case SDL_SCANCODE_S: return 0x8;
    case SDL_SCANCODE_D: return 0x9;
    case SDL_SCANCODE_F: return 0xE;
    case SDL_SCANCODE_Z: return 0xA;
    case SDL_SCANCODE_X: return 0x0;
    case SDL_SCANCODE_C: return 0xB;
    case SDL_SCANCODE_V: return 0xF;
    default: return std::nullopt;
  }
}

} // namespace

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " <rom_path> [frequency int]\n";
    return 1;
  }

  // Initialize cycle period
  int target_frequency{500};
  if (argc > 2) {
    target_frequency = std::stoi(argv[2]);
    if (target_frequency <= 0) {
      throw std::invalid_argument("Frequency must be positive");
    }
  }
  const auto cycle_period{ std::chrono::nanoseconds(1'000'000'000 / target_frequency) };

  // Initialize timer period
  constexpr int timer_frequency{60};
  constexpr auto timer_period{ std::chrono::nanoseconds(1'000'000'000 / timer_frequency) };

  // Initialize hardware
  CPU cpu;
  SDLDisplay display;
  Keypad keypad;

  cpu.load_rom(argv[1]);

  // Starting times for cycle
  auto now{ std::chrono::steady_clock::now() };
  auto next_cycle{now};
  auto next_timer{now};

  bool running{true};
  while (running) {
    // Check if user closed window or pressed/released a mapped key
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      } else if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
        auto key{ scancode_to_key(event.key.scancode) };
        if (key.has_value()) {
          if (event.type == SDL_EVENT_KEY_DOWN) {
            keypad.press(*key);
          } else {
            keypad.release(*key);
          }
        }
      }
    }
    if (!running) break;

    // Sleep until the earlier of the two deadlines
    const auto next_deadline{ std::min(next_cycle, next_timer) };
    std::this_thread::sleep_until(next_deadline);

    now = std::chrono::steady_clock::now();

    if (now >= next_cycle) {
      cpu.cycle(display, keypad);
      display.render();
      next_cycle += cycle_period;
      if (next_cycle < now) next_cycle = now; // resync, drop backlog
    }

    if (now >= next_timer) {
      cpu.updateTimers();
      next_timer += timer_period;
      if (next_timer < now) next_timer = now; // resync, drop backlog
    }
  }

  return 0;
}