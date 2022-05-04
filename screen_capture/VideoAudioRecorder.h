// modificata
#include "VideoRecorder.h"
#include "AudioRecorder.h"
#include "ffmpeg.h"
#include <thread>
#include <mutex>
#include <atomic>
//aggiunto #ifdef
#ifdef _WIN32
#include "wtypes.h"
#elif linux
#include <linux/types.h>
#endif
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

	std::mutex write_mutex;
	std::condition_variable stopped_cv;

	std::string failReason;

	std::atomic_bool isRun, isStopped;

	bool audio;
	int horizontal, vertical;

private:

	void GetDesktopResolution(int& horizontal, int& vertical);

public:

	// costruttore
	VideoAudioRecorder(std::string outputFileName, std::pair<int, int>& p_tl, std::pair<int, int>& p_br, bool audio);
	
	~VideoAudioRecorder();

	void Open();

	void Pause();

	void Restart();

	void Start();

	void Stop();

	void outputInit();

	std::string getFailureReason();
};

