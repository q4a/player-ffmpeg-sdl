#include "audio_decoder.h"
#include "ffmpeg.h"

AudioDecoder::AudioDecoder(AVCodecParameters* codec_parameters) {
	avcodec_register_all();
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

AudioDecoder::~AudioDecoder() {
	avcodec_free_context(&codec_context_);
}

bool AudioDecoder::send(AVPacket* packet) {
	auto ret = avcodec_send_packet(codec_context_, packet);
	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
		return false;
	} else {
		ffmpeg::check(ret);
		return true;
	}
}

bool AudioDecoder::receive(AVFrame* frame) {
	auto ret = avcodec_receive_frame(codec_context_, frame);
	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
		return false;
	} else {
		ffmpeg::check(ret);
		return true;
	}
}

int AudioDecoder::audio_sample_rate() const {
	return codec_context_->time_base.den;
}

int AudioDecoder::channels() const {
	return codec_context_->channels;
}

uint64_t AudioDecoder::channel_layout() const {
	return codec_context_->channel_layout;
}

AVSampleFormat AudioDecoder::format() const {
	return codec_context_->sample_fmt;
}

AVRational AudioDecoder::time_base() const {
	return codec_context_->time_base;
}
