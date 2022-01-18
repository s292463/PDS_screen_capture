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

    std::atomic_bool     *isRun;
    std::thread         *audioThread;

    

public:

    AudioRecorder(string filepath, string device, std::atomic_bool* isRun)
            :outfile(filepath),deviceName(device),failReason(""), isRun(isRun)
    {}

    

    ~AudioRecorder() {
        swr_free(&audioConverter);
        av_audio_fifo_free(audioFifo);

        avcodec_free_context(&audioInCodecCtx);
        avcodec_free_context(&audioOutCodecCtx);

        avformat_close_input(&audioInFormatCtx);
        puts("Stop record."); fflush(stdout);
    }

    void Open();
    void StartEncode(std::mutex& w_m, std::mutex& s_m, std::condition_variable& s_cv, std::atomic_bool& isStopped);
    void initializeEncoder(AVFormatContext* outputFormatContext);

    std::string GetLastError() { return failReason; }
};
#endif //AUDIORECORDER_AUDIORECORDER_H
