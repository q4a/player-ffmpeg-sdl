#include "player.h"
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
	try {
		/*
		if (argc != 2) {
			throw std::logic_error{"Not enough arguments"};
		}

		Player play{argv[1]};
		*/

		const char* filename = 
			"M:\\transfer\\use\\anime\\Clannad - 01\\[SS-Eclipse]_Clannad_-_02_(1024x576_h264)_[F952E06B].mkv"
			//"M:\\transfer\\use\\documentary_temp\\American Propaganda Films - Capitalism (1948).avi"
			//"M:\\transfer\\use\\documentary_temp\\BBC - Isaac Newton - The Dark Heretic.svcd.mpg"
			//"M:\\transfer\\use\\documentary_temp\\Discovery - Robosapiens.mpg"
			//"M:\\transfer\\use\\documentary_temp\\mysteries.of.egypt.imax.1998.dvdrip.divx-qix.avi"
			;

		Player play{ filename };
		play();
	}

	catch (const std::exception &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return -1;
	}

	return 0;
}
