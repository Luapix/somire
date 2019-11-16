#pragma once

#include <memory>
#include <stdexcept>
#include <unordered_map>

#include "ast.hpp"
#include "chunk.hpp"

class Context {
public:
	Context(Context* parent = nullptr);
	
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
	
	inline FunctionChunk& getFunc(uint32_t funcIdx) { return curChunk->functions[funcIdx]; }
	inline std::back_insert_iterator<std::vector<uint8_t>>& getCodeOut(uint32_t funcIdx) { return getFunc(funcIdx).codeOut; }
	
	void compileFunction(NodeBlock& block, std::vector<std::string> argNames);
	void compileBlock(uint32_t funcIdx, NodeBlock& block, Context& ctx);
	void compileStatement(uint32_t funcIdx, Node& stat, Context& ctx);
	void compileExpression(uint32_t funcIdx, Node& expr, Context& ctx);
	void compileConstant(uint32_t funcIdx, Value val);
};
