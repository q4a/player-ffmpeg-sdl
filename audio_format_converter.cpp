#include "audio_format_converter.h"
#include "ffmpeg.h"

extern "C"
{
	#include "libavutil/opt.h"
}

AudioFormatConverter::AudioFormatConverter(
	uint64_t channel_layout, int sample_rate, AVSampleFormat format) :
	channel_layout_{ channel_layout}, sample_rate_{ sample_rate}, format_{ format},
	conversion_context_{swr_alloc()} {

	av_opt_set_int(conversion_context_, "in_channel_layout", channel_layout_, 0);
	av_opt_set_int(conversion_context_, "out_channel_layout", channel_layout_, 0);
	av_opt_set_int(conversion_context_, "in_sample_rate", sample_rate_, 0);
	av_opt_set_int(conversion_context_, "out_sample_rate", sample_rate_, 0);
	av_opt_set_sample_fmt(conversion_context_, "in_sample_fmt", format_, 0);
	av_opt_set_sample_fmt(conversion_context_, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
	ffmpeg::check(swr_init(conversion_context_));
}

int AudioFormatConverter::convert(AVFrame* src, uint8_t **out, int out_count) {
	return swr_convert(conversion_context_, out, out_count, const_cast<const uint8_t **>(src->data), src->nb_samples);
}
