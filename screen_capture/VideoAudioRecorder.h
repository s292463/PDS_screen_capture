#include "VideoRecorder.h"
#include "AudioRecorder.h"
#include "ffmpeg.h"
#include <thread>
#include <mutex>
#include <atomic>
#include "wtypes.h"
#include <utility>

#pragma once
class VideoAudioRecorder
{

	VideoRecorder* video_recorder;
	AudioRecorder* audio_recorder;

	AVFormatContext* outputFormatContext;
	std::string outputFileName;
	std::thread* audio_thread;
	std::thread* video_thread;

	std::mutex write_mutex, stop_mutex;
	std::condition_variable stopped_cv;

	std::string failReason;

	std::atomic_bool isRun, isStopped;

	bool audio;
	int horizontal, vertical;


	void GetDesktopResolution(int& horizontal, int& vertical)
	{
		RECT desktop;
		// Get a handle to the desktop window
		const HWND hDesktop = GetDesktopWindow();
		// Get the size of screen to the variable desktop
		GetWindowRect(hDesktop, &desktop);
		// The top left corner will have coordinates (0,0)
		// and the bottom right corner will have coordinates
		// (horizontal, vertical)
		horizontal = desktop.right;
		vertical = desktop.bottom;
	}

public:

	
	// costruttore
	VideoAudioRecorder(std::string outputFileName, pair<int, int> p_tl, pair<int, int> p_br, bool audio) :
		outputFileName(outputFileName), failReason(""), audio(audio), isRun(true), isStopped(false)
	{
		GetDesktopResolution(this->horizontal, this->vertical);

		if (p_br.first > vertical || p_br.second > horizontal) {
			throw std::runtime_error("Error: portion to be registered is too big");
		}

		pair<std::string, std::string> offset = { std::to_string(p_tl.first), std::to_string(p_tl.second) };
		pair<std::string, std::string> resolution = { std::to_string(p_br.first - p_tl.first), std::to_string(p_br.second - p_tl.second) };


		video_recorder = new VideoRecorder{ outputFileName, "15", offset, resolution, &isRun};
		if (audio) {
			audio_recorder = new AudioRecorder{ outputFileName, "", &isRun };
		}
	}
	
	~VideoAudioRecorder() {


		Stop();

		if (outputFormatContext && !(outputFormatContext->oformat->flags & AVFMT_NOFILE))
		{
			avio_close(outputFormatContext->pb);
		}

		if (outputFormatContext)
			avformat_free_context(outputFormatContext);

		if (audio) delete audio_thread;
		delete video_thread;

		if (audio) delete audio_recorder;
		delete video_recorder;



	}

	void Open() {
		video_recorder->Open();
		if(this->audio)
			audio_recorder->Open();
	}

	void Pause() {
		std::cout << "Recording stopped" << std::endl;
		this->isStopped.exchange(true);
	}

	void Restart() {
		std::cout << "Recording restarted" << std::endl;

		this->isStopped.exchange(false);
		this->stopped_cv.notify_all();
	}

	void Start() {
		// TODO: Rilanciare le eccezioni nel main thread
		video_thread = new std::thread{
			[this]() {
				try {
				this->video_recorder->startCapturing(this->write_mutex, this->stop_mutex, this->stopped_cv, isStopped);
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
					this->audio_recorder->StartEncode(this->write_mutex, this->stop_mutex, this->stopped_cv, isStopped);
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
		if(this->audio)
			this->audio_thread->join();	
		
		//int ret = av_write_trailer(this->outputFormatContext);

		//if (ret < 0) throw std::runtime_error("can not write file trailer.");
		//avio_close(outputFormatContext->pb);
		
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

