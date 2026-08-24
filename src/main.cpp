#include "CPU.h"
#include "Keypad.h"
#include "TerminalDisplay.h"

#include <chrono>
#include <iostream>
#include <thread>

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
  TerminalDisplay display;
  Keypad keypad; // TODO: No input backend yet, keys are never pressed until SDL support added

  cpu.load_rom(argv[1]);

  // Starting times for cycle
  auto now{ std::chrono::steady_clock::now() };
  auto next_cycle{now};
  auto next_timer{now};

  while (true) {
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
}