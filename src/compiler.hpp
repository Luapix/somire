#pragma once

#include <memory>
#include <stdexcept>
#include <unordered_map>

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
	std::unordered_map<std::string, uint8_t> locals;
	
	void compileBlock(Chunk& chunk, NodeBlock& block);
	void compileStatement(Chunk& chunk, Node& stat);
	void compileExpression(Chunk& chunk, Node& expr);
	void compileConstant(Chunk& chunk, Value val);
};
