
#include "AudioRecorder.h"
#include <string>

using namespace std;

int main1(){
    puts("==== Audio Recorder ====");
    avdevice_register_all();

    AudioRecorder recorder{ "testAudio.aac","" };
    try {
        recorder.Open();
        recorder.Start();

        //record 10 seconds.
        std::this_thread::sleep_for(10s);

        recorder.Stop();
        string reason = recorder.GetLastError();
        if (!reason.empty()) {
            throw std::runtime_error(reason);
        }
    }
    catch (std::exception &e) {
        fprintf(stderr,"[ERROR] %s\n", e.what());
        exit(-1);
    }

    puts("END");
    return 0;
}