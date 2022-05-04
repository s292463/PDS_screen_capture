#include "VideoAudioRecorder.h"
#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <utility>

using namespace std;

int main() {
    std::puts("==== Video-Audio Recorder ====");

	bool closeProgram = false;
	char command = '\0';


	// TODO: Mettere tutti i path assoluti
	// std::string outputFileName = "./Output/output.mp4";
    std::string outputFileName = "C:/Users/elia_/OneDrive/Desktop/output.mp4"; // ELIA WINDOWS
	//std::string outputFileName = "C:/Users/chris/Desktop/output.mp4"; // CHRIS WINDOWS
    //std::string outputFileName = "/home/ilchrees/Desktop/output.mp4"; //CHRIS LINUX
	bool audio = true;
	VideoAudioRecorder* capturer = nullptr;
    char command_status;
	

	try {	//----------------------TO DO: capire se necessario deallocare dentro il catch---------------------------------------------

		// Per formare il rettangolo di registrazione
		// vengono passati il punto in alto a sinistra e il putno in basso a destra.
		// punti definiti come-> pair{x,y}
		std::pair<int, int> p1 = std::make_pair(200, 200), p2 = std::make_pair(500, 500);
		capturer = new VideoAudioRecorder{ outputFileName, p1, p2, audio };

		std::cout << "Welcome to screen capturer" << std::endl;

		capturer->Open();
		capturer->outputInit();
		capturer->Start();

		std::cout << "Capturing Started" << std::endl 
				  << "Press 's' to stop and 'r' reactivate the recording,\n 'c' to close the program\n";


		while (!closeProgram) {
			std::cin >> command;

			switch (command)
			{
			case's':
				capturer->Pause();
                command_status = command;
				break;
			case'r':
                if(command_status == 's'){
                    capturer->Restart();
                    command_status = command;
                }
                else {
                    std::cout << "Cannot restart. You should stop the system first" << std::endl;
                }
				break;
			case'c':
				capturer->Stop();
				closeProgram = true;
				break;

			default:
				std::cout << "Not available command" << std::endl;
                command_status = '\0';
				break;
			}
		}
		/*auto programFailureReason = capturer->getFailureReason();
		if (!programFailureReason.empty())
			throw std::runtime_error(programFailureReason);

		std::this_thread::sleep_for(20s);*/

	}
	catch (std::exception& e) {
		fprintf(stderr, "[ERROR] %s\n", e.what());
		exit(-1);
	}
	
	if(capturer != nullptr)
		delete capturer;

	std::cout << "END" << std::endl;
    return 0;
}
 

void parametersParser(char** argv) {

}