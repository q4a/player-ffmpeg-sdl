#pragma once
extern "C" {
	#include "libavcodec/avcodec.h"
}

class VideoDecoder {
public:
	VideoDecoder(AVCodecParameters* codec_parameters);
	~VideoDecoder();
	bool send(AVPacket* packet);
	bool receive(AVFrame* frame);
	unsigned width() const;
	unsigned height() const;
	unsigned display_width() const;
	unsigned display_height() const;
	AVPixelFormat pixel_format() const;
	AVRational time_base() const;
private:
	void get_display_size(unsigned *width, unsigned *height) const;

	AVCodecContext* codec_context_{};
};
