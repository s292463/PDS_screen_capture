#include "VideoRecorder.h"
#include "AudioRecorder.h"
#include "ffmpeg.h"
#include <thread>

#pragma once
class VideoAudioRecorder
{

	VideoRecorder* video_recorder;
	AudioRecorder* audio_recorder;

	AVFormatContext* outputFormatContext;
	std::string outputFileName;
	std::thread* audio_thread;
	std::thread* video_thread;


public:
	// costruttore
	VideoAudioRecorder(std::string outputFileName, int n_frame, std::string framerate, Resolution res, std::string offset_x, std::string offset_y, int audio):
		outputFileName(outputFileName) 
	{
		video_recorder = new VideoRecorder{ outputFileName, n_frame, framerate, res, offset_x, offset_y };
		if (audio) {
			audio_recorder = new AudioRecorder{ outputFileName,"" };
		}
	}
		

	void Open() {
		video_recorder->Open();
		audio_recorder->Open();

	}

	void start() {

	}

	int outputInit() {

		// Alloco il format context di output
		if (avformat_alloc_output_context2(&outputFormatContext, NULL, NULL, outputFileName.c_str()) < 0) {
			throw std::runtime_error("Failed to alloc ouput context");
		}
		// ADTS(Audio Data Transport Stream)
		//ret = avformat_alloc_output_context2(&audioOutFormatCtx, NULL, "adts", NULL);

		if (!outputFormatContext) //Effettuo un check
		{
			throw std::runtime_error("\nError in allocating av format output context");
		}

		// Cerca un formato che mecci il formato di output. Ritorna NULL se non ne trova uno
		if (!av_guess_format(NULL, this->outputFileName.c_str(), NULL)) //Effettuo un check
		{
			throw std::runtime_error("\nError in guessing the video format. try with correct format");
		}

		// passare l'outputFormatContext
		video_recorder->initializeEncoder(outputFormatContext);


		//Creo il file di output
		if (!(outputFormatContext->flags & AVFMT_NOFILE)) {
			//PB: I/O context
			if (avio_open2(&outputFormatContext->pb, outputFileName.c_str(), AVIO_FLAG_WRITE, NULL, NULL) < 0) {
				throw std::runtime_error("Could not open output file " + outputFileName);
			}
		}

		// istruzione successiva in AudioRecorder->Open
		//const AVCodec* audioOutCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);


		audio_recorder->initializeEncoder(outputFormatContext);

		// istruzione precedente in in AudioRecorder->Open
		//avcodec_parameters_from_context(audioOutStream->codecpar, audioOutCodecCtx);
		// 
		// Scrivo l'header sul formatContext di output
		if (avformat_write_header(outputFormatContext, NULL) < 0) {
			throw std::runtime_error("Error occurred when writing header file\n");
		}

		// Stampo le informazioni del output format context
		av_dump_format(outputFormatContext, 0, this->outputFileName.c_str(), 1);

	}


};

