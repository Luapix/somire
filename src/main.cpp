#include <iostream>
#include <fstream>
#include <cstdint>
#include <memory>
#include <string>

#include "ast.hpp"
#include "chunk.hpp"
#include "parser.hpp"
#include "compiler.hpp"
#include "vm.hpp"

int main(int argc, char const *argv[]) {
	if(argc != 3) {
		std::cout << "\nUsage: somire parse|compile [filename]" << std::endl;
		return 1;
	}
	std::string op(argv[1]);
	std::string inputPath(argv[2]);
	if(op == "parse") {
		std::ifstream inputFile(inputPath);
		if(!inputFile) {
			std::cout << "Could not open input file" << std::endl;
			return 1;
		}
		try {
			auto parser = newFileParser(inputFile);
			std::unique_ptr<Node> node = parser.parseProgram();
			std::cout << node->toString() << std::endl;
		} catch(ParseError& e) {
			std::cout << e.what() << std::endl;
			return 1;
		}
	} else if(op == "compile") {
		std::unique_ptr<Node> program;
		{
			std::cout << "Parsing..." << std::endl;
			std::ifstream inputFile(inputPath);
			if(!inputFile) {
				std::cout << "Could not open input file" << std::endl;
				return 1;
			}
			try {
				auto parser = newFileParser(inputFile);
				program = parser.parseProgram();
			} catch(ParseError& e) {
				std::cout << e.what() << std::endl;
				return 1;
			}
		}
		{
			std::cout << "Compiling..." << std::endl;
			
			Compiler compiler;
			std::unique_ptr<Chunk> chunk = compiler.compileChunk(std::move(program));
			
			std::string outputPath = inputPath.substr(0, inputPath.rfind('.')) + ".out";
			std::ofstream outputFile(outputPath, std::ios::binary);
			chunk->writeToFile(outputFile);
		}
	} else if(op == "run") {
		std::ifstream inputFile(inputPath, std::ios::binary);
		if(!inputFile) {
			std::cout << "Could not open bytecode file" << std::endl;
			return 1;
		}
		
		int status = 0;
		{
			std::unique_ptr<Chunk> chunk = Chunk::loadFromFile(inputFile);
			VM vm;
			try {
				vm.run(*chunk);
			} catch(ExecutionError& e) {
				std::cout << e.what() << std::endl;
				status = 1;
			}
		}
		GC::collect();
		GC::logState();
		return status;
	} else {
		std::cout << "Unknown operation: " << op << std::endl;
		return 1;
	}
	return 0;
}
