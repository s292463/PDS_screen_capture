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
#include <libavdevice/avdevice.h>
}

#include <iostream>
#include <fstream>

#define __STDC_CONSTANT_MACROS

int main(int argc, const char* argv[]) {

	const char *fileName = "C:/Users/chris/Desktop/VID-20161112-WA0038.mp4";
	// Allocation of the main format context where the imported media file is stored
	AVFormatContext* formatContext_p = avformat_alloc_context();

	AVDictionary* options = NULL;

	// Serve a iscrivere la nostra aplicazione per l'utilizzo di multimedia device come x11grab, dshow, gdigrab
	avdevice_register_all();

	// Option on recordig video
	//av_dict_set(&options, "framerate", "5", 0);
	//av_dict_set(&options, "offset_x", "20", 0);
	//av_dict_set(&options, "offset_y", "40", 0);
	//av_dict_set(&options, "video_size", "640x480", 0);

	//auto *pAVInputFormat = av_find_input_format("dshow");
	//auto *pAVInputFormat = av_find_input_format("gdigrab");
	
	//int value = avformat_open_input(&formatContext_p, "video=screen-capture-recorder", pAVInputFormat, NULL);
	//int value = avformat_open_input(&formatContext_p, "desktop", pAVInputFormat, &options);

	int value = avformat_open_input(&formatContext_p, fileName, nullptr, nullptr);

	if (value != 0)
	{
		std::cout << "Error in opening input device";
		exit(1);
	}


	std::cout << fileName << std::endl << "Format " << formatContext_p->iformat->long_name << "\nDuration: " << formatContext_p->duration * 0.000001 << " sec" << std::endl;
	std::cout << "Number of streams: "<<formatContext_p->nb_streams<<std::endl;

	for (int i = 0; i < formatContext_p->nb_streams; i++)
	{
		// Estraiamo i parametri del codec dello stream dati (audio/video)
		AVCodecParameters *videoCodecPar = formatContext_p->streams[i]->codecpar;
		// Tra i parametri trovati usiamo l' id del codec per trovare il decoder che ci serve
		const AVCodec *videoCodec = avcodec_find_decoder(videoCodecPar->codec_id);

		// codec_type definisce il tipo di stream su cui viene applicato il codec
		// codec_id definisce il tipo di codec in cui e'codificato lo stream 
		if (videoCodecPar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			std::cout<<"Video Codec: resolution "<<videoCodecPar->width<<"x" << videoCodecPar->height<<std::endl;
		}
		else if (videoCodecPar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			std::cout << "Audio Codec: "<< videoCodecPar->channels << " channels, Sample Rate: " << videoCodecPar->sample_rate<< std::endl;
		}

		// General info about codecs
		std::cout<<"\tCodec "<<videoCodec->long_name <<", ID: "<< videoCodec->id << ", bit_rate: "<<videoCodecPar->bit_rate << std::endl;;
	}


	return 0;
}