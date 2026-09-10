#ifndef CHIP8_EMULATOR_SPEAKER_H
#define CHIP8_EMULATOR_SPEAKER_H

#include <SDL3/SDL.h>
#include <atomic>

// Plays a continuous square-wave beep while playing is set, using an
// SDL audio stream pulled from the audio device's own thread
class Speaker {
public:
  explicit Speaker(int frequency_hz = 440, int sample_rate = 44100);
  ~Speaker();

  // Prevent copies
  Speaker(const Speaker&) = delete;
  Speaker& operator=(const Speaker&) = delete;

  void set_playing(bool playing);

private:
  static void SDLCALL audio_callback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount);
  void fill_stream(SDL_AudioStream* stream, int additional_amount);

  int m_frequency_hz;
  int m_sample_rate;
  double m_phase{0.0};
  std::atomic<bool> m_playing{false};
  SDL_AudioStream* m_stream{nullptr};
};

#endif // CHIP8_EMULATOR_SPEAKER_H