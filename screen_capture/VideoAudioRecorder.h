#include "VideoRecorder.h"
#include "AudioRecorder.h"
#include "ffmpeg.h"
#include <thread>
#include <mutex>
#include <atomic>

#pragma once
class VideoAudioRecorder
{

	VideoRecorder* video_recorder;
	AudioRecorder* audio_recorder;

	AVFormatContext* outputFormatContext;
	std::string outputFileName;
	std::thread* audio_thread;
	std::thread* video_thread;

	std::mutex m;
	std::condition_variable cv;

	std::string failReason;

	std::atomic_bool isRun;

	bool audio;




public:
	// costruttore
	VideoAudioRecorder(std::string outputFileName, std::string framerate, Resolution res, std::string offset_x, std::string offset_y, bool audio) :
		outputFileName(outputFileName), failReason(""),audio(audio), isRun(true)
	{
		video_recorder = new VideoRecorder{ outputFileName,  framerate, res, offset_x, offset_y, &isRun};
		if (audio) {
			audio_recorder = new AudioRecorder{ outputFileName, "", &isRun };
		}
	}
	
	~VideoAudioRecorder() {


		Stop();

		if (audio) delete audio_thread;
		delete video_thread;

		avformat_free_context(outputFormatContext);

		if (audio) delete audio_recorder;
		delete video_recorder;



	}

	void Open() {
		video_recorder->Open();
		if(this->audio)
			audio_recorder->Open();

	}

	void Start() {

		video_thread = new std::thread{
			[this]() {
				try {
				this->video_recorder->startCapturing(this->m, this->cv);
				}
				catch (std::exception& e) {
					this->failReason = e.what();
}
			}
		};

		if (this->audio) {
			audio_thread = new std::thread{
				[this]() {
					try {
					this->audio_recorder->StartEncode(this->m, this->cv);
					}
					catch (std::exception& e) {
						this->failReason = e.what();
					}
				}
			};
		}
		
	}

	void Stop() {
		bool r = isRun.exchange(false);
		if (!r) return; //avoid run twice

		this->video_thread->join();
		this->audio_thread->join();

		int ret = av_write_trailer(this->outputFormatContext);
		if (ret < 0) throw std::runtime_error("can not write file trailer.");
		avio_close(outputFormatContext->pb);
		
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

		if(audio)
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

	std::string getFailureReason() { return this->failReason; }
};

