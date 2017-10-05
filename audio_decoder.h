#pragma once
extern "C" {
	#include "libavcodec/avcodec.h"
}

class AudioDecoder {
public:
	AudioDecoder(AVCodecParameters* codec_parameters);
	~AudioDecoder();
	bool send(AVPacket* packet);
	bool receive(AVFrame* frame);
	int audio_sample_rate() const;
	uint64_t channel_layout() const;
	AVSampleFormat format() const;
	AVRational time_base() const;
private:
	AVCodecContext* codec_context_{};
};
