#pragma once
extern "C" {
	#include "libavformat/avformat.h"
	#include "libswresample/swresample.h"
}

class AudioFormatConverter {
public:
	AudioFormatConverter(
		uint64_t channel_layout, int sample_rate, AVSampleFormat format);
	int convert(AVFrame* src, uint8_t **out, int out_count);
private:
	uint64_t channel_layout_;
	int sample_rate_;
	AVSampleFormat format_;
	SwrContext* conversion_context_{};
};
