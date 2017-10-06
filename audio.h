#pragma once
#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include <array>
#include <memory>

class Audio {
private:
	SDL_AudioSpec want_{}, have_{};
	SDL_AudioDeviceID dev_;
public:
	Audio(const int freq, Uint8 channels);

	SDL_AudioDeviceID dev() { return dev_; }
	int bytes_per_sample() const;

	// Queue audio
	void queue(
		const void *data, Uint32 len);
};
