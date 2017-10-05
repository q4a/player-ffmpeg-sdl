#pragma once
extern "C" {
	#include "libavformat/avformat.h"
	#include "libswresample/swresample.h"
}

class AudioFormatConverter {
public:
	AudioFormatConverter(
		uint64_t channel_layout, int sample_rate, AVSampleFormat format);
	void operator()(AVFrame* src, AVFrame* dst);
private:
	uint64_t channel_layout_;
	int sample_rate_;
	AVSampleFormat format_;
	SwrContext* conversion_context_{};
};
