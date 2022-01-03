#include "VideoRecorder.h"
#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>


int main() {

	std::condition_variable cv;
	std::mutex m;
	std::unique_lock<std::mutex> ul{ m };

	bool closeProgram = false;
	int stopRecording = 0;
	Resolution fullHD = { "1920", "1080" };
	Resolution twoK = { "2560", "1440" };

	int n_frame = 100;

	//std::string outputFileName = "C:/Users/elia_/OneDrive/Desktop/output.mp4";

	std::string outputFileName = "C:/Users/chris/Desktop/output.mp4";
	std::string offset_x = "0";
	std::string offset_y = "0";
	std::string framerate = "15";

	VideoRecorder* capturer = new VideoRecorder{ outputFileName, n_frame, framerate, fullHD, offset_x, offset_y };


	while (!closeProgram) {
		try {
			std::cout << "Welcome to screen capturer" << std::endl;

			capturer->Open();
			capturer->Start();

			auto programFailureReason = capturer->getFailReason();
			if (!programFailureReason.empty())
				throw std::runtime_error(programFailureReason);

		}
		catch (std::exception& e) {
			fprintf(stderr, "[ERROR] %s\n", e.what());
			exit(-1);
		}
		closeProgram = true;
	}
	

	delete capturer;

	return 0;
}


#include "AudioRecorder.h"
#include <string>

using namespace std;

int main0() {
    puts("==== Audio Recorder ====");
    avdevice_register_all();

    AudioRecorder recorder{ "C:/Users/chris/Desktop/testAudio.aac","" };
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

 