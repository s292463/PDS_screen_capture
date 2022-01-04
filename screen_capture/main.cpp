#include "VideoAudioRecorder.h"
#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>



using namespace std;


int main() {
    puts("==== Audio Recorder ====");
    avdevice_register_all();

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
	bool audio = true;

	VideoAudioRecorder* capturer = new VideoAudioRecorder{ outputFileName, framerate, fullHD, offset_x, offset_y, audio};
	

	
	try {
		std::cout << "Welcome to screen capturer" << std::endl;

		capturer->Open();
		capturer->outputInit();
		capturer->Start();

		auto programFailureReason = capturer->getFailureReason();
		if (!programFailureReason.empty())
			throw std::runtime_error(programFailureReason);

		std::this_thread::sleep_for(20s);
		
		capturer->Stop();

	}
	catch (std::exception& e) {
		fprintf(stderr, "[ERROR] %s\n", e.what());
		exit(-1);
	}
		
	delete capturer;

    puts("END");
    return 0;
}
 