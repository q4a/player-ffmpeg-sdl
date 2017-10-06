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

Audio::Audio(const int freq, Uint8 channels)
{
	want_.freq = freq;
	want_.format = AUDIO_S16SYS;
	want_.channels = channels;
	want_.samples = 4096;
	dev_ = SDL_OpenAudioDevice(NULL, 0, &want_, &have_, 0);
	SDL_PauseAudioDevice(dev_, 0);
}

void Audio::queue(
	const void *data, Uint32 len) {
	SDL_QueueAudio(dev_, data, len);
}
