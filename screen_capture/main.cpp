#include "VideoCapture.h"
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

	int value = 0;


	std::string outputFileName = "C:/Users/chris/Desktop/output.mp4";
	std::string offset_x = "0";
	std::string offset_y = "0";
	std::string framerate = "15";

	VideoCapture* capturer = new VideoCapture{ outputFileName, framerate, fullHD, offset_x, offset_y };


	std::thread t_capturer{ 
		[&capturer, &ul, &value, &cv, &stopRecording] () {
			value = capturer->intilizeDecoder();
			value = capturer->initializeEncoder();
			value = capturer->startCapturing(100, ul, cv, stopRecording);
		}
	};

	while (!closeProgram) {
		std::cout << "Welcome to screen capturer" << std::endl << "Choose an option:" << std::endl;
		std::cin >> stopRecording;

		if (stopRecording == 1) {
			cv.notify_one();
		}
		/*else if (stopRecording == 0) {
			
		}*/
	}
	

	t_capturer.join();

	delete capturer;

	return 0;
}