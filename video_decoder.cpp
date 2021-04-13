#include "video_decoder.h"
#include "ffmpeg.h"

VideoDecoder::VideoDecoder(AVCodecParameters* codec_parameters) {
	const auto codec = avcodec_find_decoder(codec_parameters->codec_id);
	if (!codec) {
		throw ffmpeg::Error{"Unsupported video codec"};
	}
	codec_context_ = avcodec_alloc_context3(codec);
	if (!codec_context_) {
		throw ffmpeg::Error{"Couldn't allocate video codec context"};
	}
	ffmpeg::check(avcodec_parameters_to_context(
		codec_context_, codec_parameters));
	ffmpeg::check(avcodec_open2(codec_context_, codec, nullptr));
}

VideoDecoder::~VideoDecoder() {
	avcodec_free_context(&codec_context_);
}

bool VideoDecoder::send(AVPacket* packet) {
	auto ret = avcodec_send_packet(codec_context_, packet);
	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
		return false;
	} else {
		ffmpeg::check(ret);
		return true;
	}
}

bool VideoDecoder::receive(AVFrame* frame) {
	auto ret = avcodec_receive_frame(codec_context_, frame);
	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
		return false;
	} else {
		ffmpeg::check(ret);
		return true;
	}
}

unsigned VideoDecoder::width() const {
	return codec_context_->width;
}

unsigned VideoDecoder::height() const {
	return codec_context_->height;
}

unsigned VideoDecoder::display_width() const {
	unsigned width, height;
	get_display_size(&width, &height);
	return width;
}

unsigned VideoDecoder::display_height() const {
	unsigned width, height;
	get_display_size(&width, &height);
	return height;
}

void VideoDecoder::get_display_size(unsigned *width, unsigned *height) const {
	// some video files have custom aspect ratios
	double aspect_ratio;
	if (codec_context_->sample_aspect_ratio.num == 0)
		aspect_ratio = 0;
	else
		aspect_ratio = av_q2d(codec_context_->sample_aspect_ratio);

	if (aspect_ratio <= 0.0)
		aspect_ratio = 1.0;
	aspect_ratio *= (double)codec_context_->width / (double)codec_context_->height;

	*height = codec_context_->height;
	*width = lrint(*height * aspect_ratio) & ~1;
	if (*width > codec_context_->width) {
		*width = codec_context_->width;
		*height = lrint(*width / aspect_ratio) & ~1;
	}
}

AVPixelFormat VideoDecoder::pixel_format() const {
	return codec_context_->pix_fmt;
}

AVRational VideoDecoder::time_base() const {
	return codec_context_->time_base;
}
