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

bool parse(std::string inputPath, std::unique_ptr<Node>& program) {
	std::ifstream inputFile(inputPath);
	if(!inputFile) {
		std::cout << "Could not open input file" << std::endl;
		return false;
	}
	try {
		auto parser = newFileParser(inputFile);
		program = parser.parseProgram();
	} catch(ParseError& e) {
		std::cout << e.what() << std::endl;
		return false;
	}
	return true;
}

bool compile(std::unique_ptr<Node> program, std::unique_ptr<Chunk>& chunk) {
	Compiler compiler;
	try {
		chunk = compiler.compileChunk(std::move(program));
	} catch(CompileError& e) {
		std::cout << e.what() << std::endl;
		return false;
	}
	return true;
}

bool loadBytecode(std::string inputPath, std::unique_ptr<Chunk>& chunk) {
	std::ifstream inputFile(inputPath, std::ios::binary);
	if(!inputFile) {
		std::cout << "Could not open bytecode file" << std::endl;
		return false;
	}
	
	chunk = Chunk::loadFromFile(inputFile);
	return true;
}

bool run(std::unique_ptr<Chunk>& chunk) {
	VM vm;
	try {
		vm.run(*chunk);
	} catch(ExecutionError& e) {
		std::cout << e.what() << std::endl;
		return false;
	}
	return true;
}

bool doOperation(std::string op, std::string inputPath) {
	if(op == "parse") {
		std::unique_ptr<Node> program;
		if(!parse(inputPath, program)) return false;
		std::cout << program->toString() << std::endl;
	} else if(op == "compile") {
		std::cout << "Parsing..." << std::endl;
		std::unique_ptr<Node> program;
		if(!parse(inputPath, program)) return false;
		
		std::cout << "Compiling..." << std::endl;
		std::unique_ptr<Chunk> chunk;
		if(!compile(std::move(program), chunk)) return false;
		
		std::string outputPath = inputPath.substr(0, inputPath.rfind('.')) + ".sbf";
		std::ofstream outputFile(outputPath, std::ios::binary);
		chunk->writeToFile(outputFile);
	} else if(op == "list") {
		std::unique_ptr<Chunk> chunk;
		if(!loadBytecode(inputPath, chunk)) return false;
		
		std::cout << chunk->list() << std::endl;
	} else if(op == "run") {
		std::unique_ptr<Chunk> chunk;
		if(!loadBytecode(inputPath, chunk)) return false;
		
		if(!run(chunk)) return false;
	} else if(op == "interpret") {
		std::unique_ptr<Node> program;
		if(!parse(inputPath, program)) return false;
		
		std::unique_ptr<Chunk> chunk;
		if(!compile(std::move(program), chunk)) return false;
		
		if(!run(chunk)) return false;
	} else {
		std::cout << "Unknown operation: " << op << std::endl;
		return false;
	}
	return true;
}

int main(int argc, char const *argv[]) {
	if(argc != 3) {
		std::cout << "\nUsage: somire parse|compile|list|run|interpret [filename]" << std::endl;
		return 1;
	}
	
	int status = 0;
	if(!doOperation(argv[1], argv[2]))
		status = 1;
	
	GC::collect();
	GC::logState();
	return status;
}
