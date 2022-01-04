#ifndef AUDIORECORDER_AUDIORECORDER_H
#define AUDIORECORDER_AUDIORECORDER_H

#ifdef _WIN32
#include "ListAVDevices.h"
#endif

#include "ffmpeg.h"
#include <string>
#include <cstdint>
#include <iostream>

//#ifdef __linux__
#include <atomic>
#include <thread>
//#endif

using std::string;

class AudioRecorder {

private:
    string outfile;
    string deviceName;
    string failReason;

    AVFormatContext *audioInFormatCtx;
    AVStream        *audioInStream;
    AVCodecContext  *audioInCodecCtx;

    SwrContext      *audioConverter;
    AVAudioFifo     *audioFifo;

    AVFormatContext *audioOutFormatCtx;
    AVStream        *audioOutStream;
    AVCodecContext  *audioOutCodecCtx;

    std::atomic_bool     isRun;
    std::thread         *audioThread;

    void StartEncode();



public:

    AudioRecorder(string filepath, string device)
            :outfile(filepath),deviceName(device),failReason(""),isRun(false){}

    void Open();
    void Start();
    void Stop();
    void initializeEncoder(AVFormatContext* outputFormatContext);

    ~AudioRecorder() {
        Stop();
    }

    std::string GetLastError() { return failReason; }
};
#endif //AUDIORECORDER_AUDIORECORDER_H
