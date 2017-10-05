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
	Audio(const int freq);

	// Copy frame to display
	void refresh(
		std::array<uint8_t*, 3> planes, std::array<size_t, 3> pitches);
};
