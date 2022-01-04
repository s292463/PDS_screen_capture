#pragma once
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavcodec/avfft.h"

#include "libavdevice/avdevice.h"

#include "libavfilter/avfilter.h"
	//#include "libavfilter/avfiltergraph.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"

#include "libavformat/avformat.h"
#include "libavformat/avio.h"

// libav resample

#include "libavutil/opt.h"
#include "libavutil/common.h"
#include "libavutil/channel_layout.h"
#include "libavutil/imgutils.h"
#include "libavutil/mathematics.h"
#include "libavutil/samplefmt.h"
#include "libavutil/time.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "libavutil/file.h"

// lib swresample

#include "libswscale/swscale.h"
}

#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <string>
#include <vector>
#include <cstring>
#include <condition_variable>
#include <atomic>


enum Options
{
	STOP,
	PLAY,
	CLOSE,
	AUDIO_ON_OFF
};

typedef struct {
	std::string width;
	std::string height;

}Resolution;

enum StreamType
{
	VIDEO,
	AUDIO
};

class VideoRecorder
{
	std::string outputFileName;
	std::string inputFileName;

	AVFormatContext* inputFormatContext = nullptr;
	AVFormatContext* outputFormatContext = nullptr;

	const AVCodec* videoDecoder = nullptr;
	const AVCodec* videoEncoder = nullptr;

	AVCodecContext* videoDecoderContext = nullptr;
	AVCodecContext* videoEncoderContext = nullptr;

	AVStream* outStream = nullptr;
	AVStream* inStream = nullptr;

	AVDictionary* options = nullptr;

	// Capture Options
	std::string failReason;
	std::string framerate;
	std::string offset_x;
	std::string offset_y;
	Resolution res;

	std::vector<int> stream_index;

	std::thread *t_capturer;
	std::atomic_bool* isRun;

	bool audioOn;

	int num_frame;

public:

	VideoRecorder(std::string outputFileName, std::string framerate, Resolution res, std::string offset_x, std::string offset_y, std::atomic_bool* isRun);
	~VideoRecorder();
	
	void Open();
	void Start();
	void Stop();

	void intilizeDecoder();
	void initializeEncoder(AVFormatContext* outputFormatContext);
	void startCapturing(std::mutex& m, std::condition_variable& cv);

	std::string getFailReason() { return this->failReason; }

};