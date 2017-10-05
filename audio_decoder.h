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
	unsigned width() const;
	unsigned height() const;
	AVPixelFormat pixel_format() const;
	AVRational time_base() const;
private:
	AVCodecContext* codec_context_{};
};
