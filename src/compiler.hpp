#pragma once

#include <memory>
#include <stdexcept>

#include "ast.hpp"
#include "chunk.hpp"

class CompileError : public std::runtime_error {
public:
	CompileError(const std::string& what);
};

class Compiler {
public:
	Compiler();
	
	std::unique_ptr<Chunk> compileChunk(std::unique_ptr<Node> ast);
	
private:
	void compileBlock(Chunk& chunk, NodeBlock& block);
};
