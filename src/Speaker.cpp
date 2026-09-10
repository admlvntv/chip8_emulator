#include "Speaker.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
  constexpr float AMPLITUDE{0.25f}; // volume
}

Speaker::Speaker(int frequency_hz, int sample_rate)
    : m_frequency_hz{frequency_hz}, m_sample_rate{sample_rate} {
  if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
    throw std::runtime_error(std::string("SDL_InitSubSystem failed: ") + SDL_GetError());
  }

  const SDL_AudioSpec spec{SDL_AUDIO_F32, 1, m_sample_rate};
  m_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, audio_callback, this);
  if (!m_stream) {
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    throw std::runtime_error(std::string("SDL_OpenAudioDeviceStream failed: ") + SDL_GetError());
  }

  SDL_ResumeAudioStreamDevice(m_stream);
}

Speaker::~Speaker() {
  if (m_stream) {
    SDL_DestroyAudioStream(m_stream);
  }
  SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void Speaker::set_playing(bool playing) {
  m_playing = playing;
}

void SDLCALL Speaker::audio_callback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount) {
  // recovers the Speaker instance from userdata and forwards to fill_stream.
  static_cast<Speaker*>(userdata)->fill_stream(stream, additional_amount);
}

void Speaker::fill_stream(SDL_AudioStream* stream, int additional_amount) {
  if (additional_amount <= 0) return;

  // additional_amount is in bytes, convert to a count of float samples
  const int sample_count{additional_amount / static_cast<int>(sizeof(float))};
  std::vector<float> samples(sample_count);

  if (m_playing) {
    // Square wave via a phase accumulator: m_phase sweeps 0.0 -> 1.0 once
    // per cycle, advancing by phase_step each sample so the cycle takes
    // exactly sample_rate / frequency_hz samples regardless of sample rate
    const double phase_step{static_cast<double>(m_frequency_hz) / m_sample_rate};
    for (int i{0}; i < sample_count; ++i) {
      // First half of the cycle is +AMPLITUDE, second half is -AMPLITUDE.
      samples[i] = (m_phase < 0.5) ? AMPLITUDE : -AMPLITUDE;
      m_phase += phase_step;
      if (m_phase >= 1.0) m_phase -= 1.0; // wrap back into [0, 1)
    }
  } else {
    // Not beeping: keep the stream fed with silence
    std::fill(samples.begin(), samples.end(), 0.0f);
  }

  // Hand the generated samples off to SDL to actually play
  SDL_PutAudioStreamData(stream, samples.data(), sample_count * static_cast<int>(sizeof(float)));
}