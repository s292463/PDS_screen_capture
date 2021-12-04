/*
 * http://ffmpeg.org/doxygen/trunk/index.html
 *
 * Main components
 *
 * Format (Container) - a wrapper, providing sync, metadata and muxing for the streams.
 * Stream - a continuous stream (audio or video) of data over time.
 * Codec - defines how data are enCOded (from Frame to Packet)
 *        and DECoded (from Packet to Frame).
 * Packet - are the data (kind of slices of the stream data) to be decoded as raw frames.
 * Frame - a decoded raw frame (to be encoded or filtered).
 */

extern "C" 
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <iostream>
#include <fstream>


int main(int argc, const char* argv[]) {

	const char *fileName = "C:/Users/chris/Desktop/VID-20161112-WA0038.mp4";
	// Allocation of the main format context where the imported media file is stored
	AVFormatContext* formatContext_p = avformat_alloc_context();

	AVDictionary* options = NULL;


	//av_dict_set(&options, "framerate", "5", 0);
	//av_dict_set(&options, "offset_x", "20", 0);
	//av_dict_set(&options, "offset_y", "40", 0);
	//av_dict_set(&options, "video_size", "640x480", 0);

	//const AVInputFormat *pAVInputFormat = av_find_input_format("dshow");
	//int value = avformat_open_input(&formatContext_p, "video=screen-capture-recorder", pAVInputFormat, NULL);
	//int value = avformat_open_input(&formatContext_p, "desktop", pAVInputFormat, &options);

	int value = avformat_open_input(&formatContext_p, fileName, nullptr, nullptr);

	if (value != 0)
	{
		std::cout << "Error in opening input device";
		exit(1);
	}


	std::cout << fileName << std::endl << "Format " << formatContext_p->iformat->long_name << "\nDuration: " << formatContext_p->duration * 0.000001 << " sec" << std::endl;

	return 0;
}