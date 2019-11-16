#pragma once

#include <memory>
#include <stdexcept>
#include <unordered_map>

#include "ast.hpp"
#include "chunk.hpp"

class Context {
public:
	Context(Context* parent);
	
	bool isInner(std::string var);
	uint16_t innerCount();
	
	void define(std::string var);
	bool isDefined(std::string var);
	uint16_t getIndex(std::string var);
	
private:
	Context* parent;
	std::unordered_map<std::string, uint16_t> innerLocals;
	
	uint16_t nextIndex();
};

class Compiler {
public:
	Compiler();
	
	std::unique_ptr<Chunk> compileProgram(std::unique_ptr<Node> ast);
	
private:
	std::unique_ptr<Chunk> curChunk;
	
	void compileFunction(NodeBlock& block);
	void compileBlock(FunctionChunk& curFunc, NodeBlock& block, Context* parent = nullptr);
	void compileStatement(FunctionChunk& curFunc, Node& stat, Context& ctx);
	void compileExpression(FunctionChunk& curFunc, Node& expr, Context& ctx);
	void compileConstant(FunctionChunk& curFunc, Value val);
};
