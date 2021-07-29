#include "player.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
extern "C" {
	#include <libavutil/time.h>
	#include <libavutil/imgutils.h>
}

const size_t Player::video_queue_size_{5};
const size_t Player::audio_queue_size_{5};

Player::Player(const std::string &file_name) :
	infolock_{},
	video_clock_{ 0 },
	audio_clock_{ 0 },

	demuxer_{std::make_unique<Demuxer>(file_name)},
	video_decoder_{std::make_unique<VideoDecoder>(
		demuxer_->video_codec_parameters())},
	audio_decoder_{ std::make_unique<AudioDecoder>(
		demuxer_->audio_codec_parameters()) },
	format_converter_{std::make_unique<FormatConverter>(
		video_decoder_->width(), video_decoder_->height(),
		video_decoder_->pixel_format(), AV_PIX_FMT_YUV420P)},
	display_{std::make_unique<Display>(
		video_decoder_->display_width(), video_decoder_->display_height(),
		video_decoder_->width(), video_decoder_->height())},
	audio_{ std::make_unique<Audio>(
		audio_decoder_->audio_sample_rate(), audio_decoder_->channels()) },
	audio_format_converter_{ std::make_unique<AudioFormatConverter>(
		audio_decoder_->channel_layout(), audio_decoder_->audio_sample_rate(), audio_decoder_->format()) },
	timer_{std::make_unique<Timer>()},
	video_packet_queue_{std::make_unique<PacketQueue>(video_queue_size_)},
	frame_queue_{std::make_unique<FrameQueue>(video_queue_size_)},
	audio_packet_queue_{ std::make_unique<PacketQueue>(audio_queue_size_) } {
}

void Player::operator()() {
	stages_.emplace_back(&Player::demultiplex, this);
	stages_.emplace_back(&Player::decode_video, this);
	stages_.emplace_back(&Player::decode_audio, this);
	video();

	for (auto &stage : stages_) {
		stage.join();
	}

	if (exception_) {
		std::rethrow_exception(exception_);
	}
}

void Player::demultiplex() {
	try {
		Uint32 audio_ticks = SDL_GetTicks();
		for (;;) {
			// Create AVPacket
			std::unique_ptr<AVPacket, std::function<void(AVPacket*)>> packet{
				av_packet_alloc(),
				[](AVPacket* p){ av_packet_free(&p); }};
			packet->data = nullptr;

			// Read frame into AVPacket
			if (!(*demuxer_)(*packet)) {
				video_packet_queue_->finished();
				audio_packet_queue_->finished();
				break;
			}

			// Move into queue if first video stream
			if (packet->stream_index == demuxer_->video_stream_index()) {
				if (!video_packet_queue_->push(move(packet))) {
					break;
				}
			}
			else if (packet->stream_index == demuxer_->audio_stream_index()) {
				if (!audio_packet_queue_->push(move(packet))) {
					break;
				}
				if (SDL_TICKS_PASSED(SDL_GetTicks(), audio_ticks + 2000))
				{
					audio_ticks = SDL_GetTicks();
					//std::cout << "-- AUDIO QUEUE SIZE: " << audio_packet_queue_->size() << std::endl;
				}
			}
		}
	} catch (...) {
		exception_ = std::current_exception();
		frame_queue_->quit();
		video_packet_queue_->quit();
		audio_packet_queue_->quit();
	}
}

void Player::decode_video() {
	try {
		const AVRational microseconds = {1, 1000000};

		Uint32 decode_ticks = SDL_GetTicks();

		for (;;) {
			// Create AVFrame and AVQueue
			std::unique_ptr<AVFrame, std::function<void(AVFrame*)>>
				frame_decoded{
					av_frame_alloc(), [](AVFrame* f){ av_frame_free(&f); }};
			std::unique_ptr<AVPacket, std::function<void(AVPacket*)>> packet{
				nullptr, [](AVPacket* p){ av_packet_unref(p); delete p; }};

			// Read packet from queue
			if (!video_packet_queue_->pop(packet)) {
				frame_queue_->finished();
				break;
			}

			// If the packet didn't send, receive more frames and try again
			bool sent = false;
			while (!sent) {
				sent = video_decoder_->send(packet.get());

				// If a whole frame has been decoded,
				// adjust time stamps and add to queue
				while (video_decoder_->receive(frame_decoded.get())) {
					frame_decoded->pts = av_rescale_q(
						frame_decoded->pkt_dts,
						demuxer_->video_time_base(),
						microseconds);

					std::unique_ptr<AVFrame, std::function<void(AVFrame*)>>
						frame_converted{
							av_frame_alloc(),
							[](AVFrame* f){ av_free(f->data[0]); }};
					if (av_frame_copy_props(frame_converted.get(),
						frame_decoded.get()) < 0) {
						throw std::runtime_error("Copying frame properties");
					}
					if (av_image_alloc(
						frame_converted->data, frame_converted->linesize,
						video_decoder_->width(), video_decoder_->height(),
						video_decoder_->pixel_format(), 1) < 0) {
						throw std::runtime_error("Allocating picture");
					}
					(*format_converter_)(
						frame_decoded.get(), frame_converted.get());

					if (!frame_queue_->push(move(frame_converted))) {
						break;
					}

					if (SDL_TICKS_PASSED(SDL_GetTicks(), decode_ticks + 1000))
					{
						decode_ticks = SDL_GetTicks();
						//std::cout << "-- FRAME QUEUE SIZE: " << frame_queue_->size() << std::endl;
					}

				}
			}
		}
	} catch (...) {
		exception_ = std::current_exception();
		frame_queue_->quit();
		video_packet_queue_->quit();
	}
}

void Player::decode_audio() {
	try {
		const AVRational microseconds = { 1, 1000000 };

		for (;;) {
			// Create AVFrame and AVQueue
			std::unique_ptr<AVFrame, std::function<void(AVFrame*)>>
				frame_decoded{
				av_frame_alloc(), [](AVFrame* f) { av_frame_free(&f); } };
			std::unique_ptr<AVPacket, std::function<void(AVPacket*)>> packet{
				nullptr, [](AVPacket* p) { av_packet_unref(p); delete p; } };

			// Read packet from queue
			if (!audio_packet_queue_->pop(packet)) {
				//frame_queue_->finished();
				break;
			}

			// If the packet didn't send, receive more frames and try again
			bool sent = false;
			while (!sent) {
				sent = audio_decoder_->send(packet.get());

				// If a whole frame has been decoded,
				// adjust time stamps and add to queue
				while (audio_decoder_->receive(frame_decoded.get())) {
					//frame->pts = av_frame_get_best_effort_timestamp(frame);
					frame_decoded->pts = av_rescale_q(
						frame_decoded->pkt_dts,
						demuxer_->audio_time_base(),
						microseconds);

					// calculate delay based on audio queue size and output bytes per second
					double bytes_per_second = (double)(audio_decoder_->audio_sample_rate() * audio_decoder_->channels() * audio_->bytes_per_sample());
					double xdelay = SDL_GetQueuedAudioSize(audio_->dev()) / bytes_per_second;
					{
						std::lock_guard<std::mutex> lock(infolock_);
						audio_clock_ = frame_decoded->pts - static_cast<int64_t>(xdelay * 1000 * 1000);
					}

					uint8_t *output;
					int out_samples = frame_decoded->nb_samples;
					if (av_samples_alloc(&output, NULL, audio_decoder_->channels(), out_samples, AV_SAMPLE_FMT_S16, 0) >= 0)
					{
						out_samples = audio_format_converter_->convert(frame_decoded.get(), &output, out_samples);
						int bytes_per_sample = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
						size_t unpadded_linesize = out_samples * bytes_per_sample;

						//audio_->queue(output, unpadded_linesize * 2);
						audio_->queue(output, unpadded_linesize * audio_decoder_->channels());

						av_freep(&output);
					}
				}
			}
		}
	}
	catch (...) {
		exception_ = std::current_exception();
		audio_packet_queue_->quit();
	}
}

void Player::video() {
	try {
		int64_t last_pts = 0;

		Uint32 video_ticks = SDL_GetTicks();

		for (uint64_t frame_number = 0;; ++frame_number) {

			display_->input();

			if (display_->get_quit()) {
				break;

			} else if (display_->get_play()) {
				std::unique_ptr<AVFrame, std::function<void(AVFrame*)>> frame{
					nullptr, [](AVFrame* f){ av_frame_free(&f); }};
				if (!frame_queue_->pop(frame)) {
					break;
				}

				if (frame_number) {
					const int64_t frame_delay = frame->pts - last_pts;
					last_pts = frame->pts;
					timer_->wait(frame_delay);

				} else {
					last_pts = frame->pts;
					timer_->update();
				}

				{
					std::lock_guard<std::mutex> lock(infolock_);
					video_clock_ = last_pts;
				}

				if (SDL_TICKS_PASSED(SDL_GetTicks(), video_ticks + 1000))
				{
					std::lock_guard<std::mutex> lock(infolock_);

					video_ticks = SDL_GetTicks();
					//std::cout << "-- VIDEO PTS: " << (frame->pts/1000/1000.0) << std::endl;
					std::cout << "-- VIDEO PTS: " << (video_clock_ / 1000 / 1000.0) << std::endl;
					std::cout << "-- AUDIO PTS: " << (audio_clock_ / 1000 / 1000.0) << std::endl;
					std::cout << "-- DIFF PTS: " << (video_clock_ / 1000 / 1000.0) - (audio_clock_ / 1000 / 1000.0) << std::endl;
					std::cout << "---" << std::endl;
				}

				display_->refresh(
					{frame->data[0], frame->data[1], frame->data[2]},
					{static_cast<size_t>(frame->linesize[0]),
					 static_cast<size_t>(frame->linesize[1]),
					 static_cast<size_t>(frame->linesize[2])});

			} else {
				std::chrono::milliseconds sleep(10);
				std::this_thread::sleep_for(sleep);
				timer_->update();
			}
		}

	} catch (...) {
		exception_ = std::current_exception();
	}

	frame_queue_->quit();
	video_packet_queue_->quit();
	audio_packet_queue_->quit();
}
