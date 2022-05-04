// modificato
#ifndef AUDIORECORDER_AUDIORECORDER_H
#define AUDIORECORDER_AUDIORECORDER_H

#ifdef _WIN32
#include "ListAVDevices.h"
#endif

#include "ffmpeg.h"
#include <string>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>


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
    //aggiunto const
    const AVInputFormat   *inputFormat;

    std::atomic_bool     *isRun;
    std::thread         *audioThread;

    

public:

    AudioRecorder(string filepath, string device, std::atomic_bool* isRun);
    ~AudioRecorder();

    void Open();
    void Reopen();
    void StartEncode(std::mutex& write_mutex, std::condition_variable& s_cv, std::atomic_bool& isStopped);
    void initializeEncoder(AVFormatContext* outputFormatContext);

    std::string GetLastError() { return failReason; }
};
#endif //AUDIORECORDER_AUDIORECORDER_H
