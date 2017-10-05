#include "audio.h"
#include <stdexcept>

template <typename T>
inline T check_SDL(T value, const std::string &message) {
	if (!value) {
		throw std::runtime_error{"SDL " + message};
	} else {
		return value;
	}
}

Audio::Audio(const int freq)
{
	want_.freq = freq;
	want_.format = AUDIO_S16SYS;
	want_.channels = 2;
	want_.samples = 4096;
	dev_ = SDL_OpenAudioDevice(NULL, 0, &want_, &have_, 0);
	SDL_PauseAudioDevice(dev_, 0);
}

void Audio::refresh(
	std::array<uint8_t*, 3> planes, std::array<size_t, 3> pitches) {
}