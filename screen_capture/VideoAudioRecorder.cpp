#include "VideoAudioRecorder.h"
#include <libavformat/avio.h>


void VideoAudioRecorder::GetDesktopResolution(int& horizontal, int& vertical)
{
	/*RECT desktop;
	// Get a handle to the desktop window
	const HWND hDesktop = GetDesktopWindow();
	// Get the size of screen to the variable desktop
	GetWindowRect(hDesktop, &desktop);
	// The top left corner will have coordinates (0,0)
	// and the bottom right corner will have coordinates
	// (horizontal, vertical)
	horizontal = desktop.right;
	vertical = desktop.bottom;*/
}

VideoAudioRecorder::VideoAudioRecorder(std::string outputFileName, std::pair<int, int>& p_tl, std::pair<int, int>& p_br, bool audio) :
	outputFileName(outputFileName), failReason(""), audio(audio), isRun(true), isStopped(false)
{
	/*GetDesktopResolution(this->horizontal, this->vertical);

	if (p_br.first > horizontal || p_br.second > vertical)
	{
		//range_error
		throw std::runtime_error("Error: this screen portion is too big to be registered ");
	}
	else if (p_br.first < 0 || p_br.second < 0) {
		throw std::runtime_error("Error: You cant insert negative points ");
	}*/

	std::pair<std::string, std::string> offset = std::make_pair(std::to_string(p_tl.first), std::to_string(p_tl.second));
	std::pair<std::string, std::string> resolution = { std::to_string(p_br.first - p_tl.first), std::to_string(p_br.second - p_tl.second) };


	video_recorder = new VideoRecorder{ outputFileName, "15", offset, resolution, &isRun };
	if (audio) {
		audio_recorder = new AudioRecorder{ outputFileName, "", &isRun };
	}
}

VideoAudioRecorder::~VideoAudioRecorder() {


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

void VideoAudioRecorder::Open() {
	video_recorder->Open();
	if (this->audio)
		audio_recorder->Open();
}

void VideoAudioRecorder::Pause() {
	std::cout << "Recording is paused" << std::endl;
	this->isStopped.exchange(true);
}

void VideoAudioRecorder::Restart() {
	std::cout << "Recording restarted" << std::endl;
	// Ri-apri video input
	this->video_recorder->Reopen();
	// Ri-apri audio input
    if(this->audio)
	    this->audio_recorder->Reopen();

	this->isStopped.exchange(false);
	this->stopped_cv.notify_all();
}

void VideoAudioRecorder::Start() {
	// TODO: Rilanciare le eccezioni nel main thread
	video_thread = new std::thread{
		[this]() {
			try {
			this->video_recorder->startCapturing(this->write_mutex, this->stopped_cv, isStopped);
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
				this->audio_recorder->StartEncode(this->write_mutex, this->stopped_cv, isStopped);
				}
				catch (std::exception& e) {
					this->failReason = e.what();
				}
			}
		};
	}

}

void VideoAudioRecorder::Stop() {
	bool r = isRun.exchange(false);
	if (!r) return; //avoid run twice

	isStopped.store(false);
	this->stopped_cv.notify_all();

	this->video_thread->join();
	if (this->audio)
		this->audio_thread->join();


	if (av_write_trailer(this->outputFormatContext) < 0)
		throw std::runtime_error("can not write file trailer.");

}

void VideoAudioRecorder::outputInit() {

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

	if (audio)
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

std::string VideoAudioRecorder::getFailureReason() { return this->failReason; }