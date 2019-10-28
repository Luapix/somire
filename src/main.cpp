#include <iostream>
#include <fstream>
#include <cstdint>

#include "parser.hpp"
#include "compiler.hpp"

int main(int argc, char const *argv[]) {
	if(argc != 3) {
		std::cout << "\nUsage: somire parse|compile [filename]" << std::endl;
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
	} else if(op == "compile") {
		std::string inputPath(argv[2]);
		std::unique_ptr<Node> program;
		{
			std::cout << "Parsing..." << std::endl;
			std::ifstream inputFile(inputPath);
			try {
				auto parser = newFileParser(inputFile);
				program = parser.parseProgram();
			} catch(ParseError& e) {
				std::cout << e.what() << std::endl;
				return 1;
			}
		}
		std::string outputPath = inputPath.substr(0, inputPath.rfind('.')) + ".out";
		{
			std::cout << "Compiling..." << std::endl;
			std::ofstream outputFile(outputPath, std::ios::binary);
			Compiler compiler(std::move(program));
			compiler.writeProgram(outputFile);
		}
	} else {
		std::cout << "Unknown operation: " << op << std::endl;
		return 1;
	}
	return 0;
}
