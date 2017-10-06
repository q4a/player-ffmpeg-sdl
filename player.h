#pragma once
#include "demuxer.h"
#include "display.h"
#include "audio.h"
#include "format_converter.h"
#include "audio_format_converter.h"
#include "queue.h"
#include "timer.h"
#include "video_decoder.h"
#include "audio_decoder.h"
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
extern "C" {
	#include <libavcodec/avcodec.h>
}

class Player {
public:
	Player(const std::string &file_name);
	void operator()();
private:
	void demultiplex();
	void decode_video();
	void decode_audio();
	void video();
private:
	std::mutex infolock_;
	int64_t video_clock_, audio_clock_;

	std::unique_ptr<Demuxer> demuxer_;
	std::unique_ptr<VideoDecoder> video_decoder_;
	std::unique_ptr<AudioDecoder> audio_decoder_;
	std::unique_ptr<FormatConverter> format_converter_;
	std::unique_ptr<AudioFormatConverter> audio_format_converter_;
	std::unique_ptr<Display> display_;
	std::unique_ptr<Audio> audio_;
	std::unique_ptr<Timer> timer_;
	std::unique_ptr<PacketQueue> video_packet_queue_;
	std::unique_ptr<FrameQueue> frame_queue_;
	std::unique_ptr<PacketQueue> audio_packet_queue_;
	std::vector<std::thread> stages_;
	static const size_t video_queue_size_;
	static const size_t audio_queue_size_;
	std::exception_ptr exception_{};
};
