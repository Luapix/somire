#include <iostream>
#include <fstream>
#include <cstdint>

#include "parser.hpp"

int main(int argc, char const *argv[]) {
	if(argc != 3) {
		std::cout << "\nUsage: somire parse [filename]" << std::endl;
		return 1;
	}
	std::string op(argv[1]);
	if(op == "parse") {
		std::ifstream fs(argv[2]);
		try {
			auto parser = newFileParser(fs);
			std::unique_ptr<Node> node = parser.parseProgram();
			std::cout << node->toString() << std::endl;
		} catch(ParseError& e) {
			std::cout << e.what() << std::endl;
			return 1;
		}
	} else {
		std::cout << "Unknown operation: " << op << std::endl;
		return 1;
	}
	return 0;
}
