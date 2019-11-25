#pragma once

#include <memory>
#include <stdexcept>
#include <unordered_map>

#include "parser/ast.hpp"
#include "chunk.hpp"
#include "types.hpp"


class Context {
public:
	Context(bool isFuncTop, Context* parent);
	
	bool getVariable(std::string var, int16_t& idx);
	void defineLocal(std::string var);
	uint16_t getLocalCount();
	
	std::vector<int16_t>& getFunctionUpvalues();
	
private:
	bool isFuncTop;
	Context* parent;
	
	std::unordered_map<std::string, int16_t> variables;
	std::vector<int16_t> upvalues;
	int16_t nextLocal;
	int16_t nextUpvalue;
	uint16_t innerLocalCount;
};

class Compiler {
public:
	Compiler();
	
	std::unique_ptr<Chunk> compileProgram(std::unique_ptr<Node> ast);
	
private:
	std::unique_ptr<Chunk> curChunk;
	
	std::vector<int16_t> compileFunction(NodeBlock& block, std::vector<std::string> argNames, Context* parent = nullptr);
	void compileBlock(FunctionChunk& curFunc, NodeBlock& block, Context& ctx, bool popLocals = true);
	void compileStatement(FunctionChunk& curFunc, Node& stat, Context& ctx);
	Type& compileExpression(FunctionChunk& curFunc, Node& expr, Context& ctx);
	void compileConstant(FunctionChunk& curFunc, Value val);
};
