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


typedef struct {
	const char* height;
	const char* width;
}resolution;

class VideoCapture
{
	const char* outputFileName;

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
	const char* framerate;
	const char* offset_x;
	const char* offset_y;

	std::vector<int> stream_index;

	bool audioOn;

	

	explicit VideoCapture(char* outputFileName, resolution res, const char* offset_x, const char* offset_y) :
		outputFileName(outputFileName),
		audioOn(true),
		framerate("30"),
		
		offset_x{ offset_x },
		offset_y{ offset_y },
		stream_index{0,0}
	{
		avdevice_register_all(); // It's not thread safe
	}


	int intilizeDecoder() {

		const AVInputFormat* pAVInputFormat = av_find_input_format("gdigrab");
		if (!pAVInputFormat) {
			std::cout << "Error in opening input device";
			exit(1);
		}

		// Option dictionary
		av_dict_set(&options, "framerate", this->framerate, 0);
		av_dict_set(&options, "preset", "medium", 0);
		av_dict_set(&options, "offset_x", this->offset_x, 0);
		av_dict_set(&options, "offset_y", this->offset_y, 0);
		av_dict_set(&options, "video_size", "1920x1080", 0);
		av_dict_set(&options, "probesize", "20M", 0);


		// alloc AVFormatContext input
		inputFormatContext = avformat_alloc_context();
		if (!inputFormatContext) {
			std::cout << "Can't allocate input context" << std::endl;
			std::exit(1);
		}
		if (avformat_open_input(&inputFormatContext, "desktop", pAVInputFormat, &options) < 0) {
			std::cout << "Can't open input context" << std::endl;
			std::exit(1);
		}

	}

};

