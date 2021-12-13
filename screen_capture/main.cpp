#include "VideoCapture.h"
#include <iostream>


int main() {

	Resolution res = { "1920", "1080" };
	std::string outputFileName = "C:/Users/chrees/Desktop/output.mp4";
	std::string offset_x = "0";
	std::string offset_y = "0";

	VideoCapture capturer = { outputFileName, res, offset_x, offset_y };


	int value = capturer.intilizeDecoder();
	value = capturer.initializeEncoder();
	value = capturer.startCapturing(100);

	return 0;
}