#include "VideoCapture.h"
#include <iostream>


int ciccio() {

	Resolution res = { "1920", "1080" };
	std::string outputFileName = "C:/Users/elia_/OneDrive/Desktop/output.mp4";
	//std::string outputFileName = "C:/Users/chrees/Desktop/output.mp4";
	std::string offset_x = "0";
	std::string offset_y = "0";

	VideoCapture capturer = { outputFileName, res, offset_x, offset_y };


	int value = capturer.intilizeDecoder();
	value = capturer.initializeEncoder();
	value = capturer.startCapturing(100);

	return 0;
}


#include "AudioRecorder.h"
#include <string>

using namespace std;

int main() {
    puts("==== Audio Recorder ====");
    avdevice_register_all();

    AudioRecorder recorder{ "C:/Users/elia_/OneDrive/Desktop/testAudio.aac","" };
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
    catch (std::exception& e) {
        fprintf(stderr, "[ERROR] %s\n", e.what());
        exit(-1);
    }

    puts("END");
    return 0;
}