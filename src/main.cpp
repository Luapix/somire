#include <iostream>
#include <fstream>
#include <cstdint>

#include "parser.hpp"

int main(int argc, char const *argv[]) {
	if(argc != 2) {
		std::cout << "\nUsage: parser [filename]\n" << std::endl;
		return 1;
	}
	std::ifstream fs(argv[1]);
	try {
		auto parser = newFileParser(fs);
		std::cout << parser.lexProgram()->toString() << std::endl;
	} catch(ParseError& e) {
		std::cout << e.what() << std::endl;
		return 1;
	}
	
	return 0;
}
