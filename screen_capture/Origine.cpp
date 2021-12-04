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
	const AVCodec* videoCodec = nullptr;
	AVCodecParameters* videoCodecPar = nullptr;
	const AVCodec* audioCodec = nullptr;
	AVCodecParameters* audioCodecPar = nullptr;

	AVDictionary* options = nullptr;

	int video_stream_index = -1;

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

	for (unsigned int i = 0; i < formatContext_p->nb_streams; i++)
	{
		// Estraiamo i parametri del codec dello stream dati (audio/video)
		
		AVMediaType type = formatContext_p->streams[i]->codecpar->codec_type;
		

		// codec_type definisce il tipo di stream su cui viene applicato il codec
		// codec_id definisce il tipo di codec in cui e'codificato lo stream 
		if (type == AVMEDIA_TYPE_VIDEO)
		{
			video_stream_index = i;
			// Estraiamo i parametri del codec dello stream dati (audio/video)
			videoCodecPar = formatContext_p->streams[i]->codecpar;
			// Tra i parametri trovati usiamo l' id del codec per trovare il decoder che ci serve
			videoCodec = avcodec_find_decoder(videoCodecPar->codec_id);
			std::cout<<"Video Codec: resolution "<<videoCodecPar->width<<"x" << videoCodecPar->height<<std::endl;
		}
		else if (type == AVMEDIA_TYPE_AUDIO)
		{
			// Estraiamo i parametri del codec dello stream dati (audio/video)
			audioCodecPar = formatContext_p->streams[i]->codecpar;
			// Tra i parametri trovati usiamo l' id del codec per trovare il decoder che ci serve
			audioCodec = avcodec_find_decoder(videoCodecPar->codec_id);
			std::cout << "Audio Codec: "<< videoCodecPar->channels << " channels, Sample Rate: " << videoCodecPar->sample_rate<< std::endl;
		}

		// General info about codecs
		std::cout<<"\tCodec "<<videoCodec->long_name <<", ID: "<< videoCodec->id << ", bit_rate: "<<videoCodecPar->bit_rate << std::endl;;

	}

	AVCodecContext* videoCodecContext = avcodec_alloc_context3(videoCodec);
	avcodec_parameters_to_context(videoCodecContext,videoCodecPar);

	// Inizializziamo il codecContext con il codec aprendolo
	avcodec_open2(videoCodecContext, videoCodec, NULL);
	/*
	AVCodecContext* audioCodecContext = avcodec_alloc_context3(audioCodec);
	avcodec_parameters_to_context(audioCodecContext,audioCodecPar);
	*/

	AVPacket* pPacket = av_packet_alloc();
	AVFrame* pFrame = av_frame_alloc();

	// av_read_frame legge i pacchetti del formatContext sequenzialmente come una readFile
	while (av_read_frame(formatContext_p, pPacket) >= 0) {
		// Poiche' il formatContext e' uno solo dobbiamo distinguere noi da quale
		// stream arriva il paccketto in modo da processarlo correttamente
		if (pPacket->stream_index == video_stream_index) {
			// Letto il pacchetto(=frame compresso) lo mandiamo al decoder attraverso
			// il codecContext
			if (avcodec_send_packet(videoCodecContext, pPacket) != 0) {
				std::cout << "Error on sending packet";
				exit(1);
			}

			avcodec_receive_frame(videoCodecContext, pFrame);

			std::cout << "Frame Number: " << videoCodecContext->frame_number << std::endl;
		}
	}







	// Close format context input
	avformat_close_input(&formatContext_p);
	av_packet_free(&pPacket);
	av_frame_free(&pFrame);
	// Close codec Context
	avcodec_free_context(&videoCodecContext);

	return 0;
}