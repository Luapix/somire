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

class Context {
public:
	Context(Context* parent);
	
	bool isInner(std::string var);
	uint8_t innerCount();
	
	void define(std::string var);
	uint8_t getIndex(std::string var);
	
private:
	Context* parent;
	std::unordered_map<std::string, uint8_t> innerLocals;
	
	uint8_t nextIndex();
};

class Compiler {
public:
	Compiler();
	
	std::unique_ptr<Chunk> compileChunk(std::unique_ptr<Node> ast);
	
private:
	void compileBlock(Chunk& chunk, NodeBlock& block, Context* parent = nullptr);
	void compileStatement(Chunk& chunk, Node& stat, Context& ctx);
	void compileExpression(Chunk& chunk, Node& expr, Context& ctx);
	void compileConstant(Chunk& chunk, Value val);
};
