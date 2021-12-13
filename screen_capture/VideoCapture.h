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


typedef struct {
	std::string width;
	std::string height;
}Resolution;

enum StreamType
{
	VIDEO,
	AUDIO
};

class VideoCapture
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
	std::string framerate;
	std::string offset_x;
	std::string offset_y;
	Resolution res;

	std::vector<int> stream_index;	
	bool audioOn;

public:

	VideoCapture(std::string outputFileName, Resolution res, std::string offset_x, std::string offset_y);
	~VideoCapture();
	
	int intilizeDecoder();
	int initializeEncoder();
	int startCapturing(int n_frame);

};