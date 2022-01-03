#include "VideoRecorder.h"
#include "AudioRecorder.h"
#include "ffmpeg.h"

#pragma once
class VideoAudioRecorder
{

	VideoRecorder* video_recorder;

	AVFormatContext* outFormatContext;


public:

	VideoAudioRecorder() {};

	void Open() {
		video_recorder->Open();
	}

};

