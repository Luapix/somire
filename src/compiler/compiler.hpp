#pragma once

#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <optional>

#include "parser/ast.hpp"
#include "chunk.hpp"
#include "types.hpp"
#include "vm/std.hpp"


struct Variable {
	int16_t idx;
	Type* type;
};

class Context {
public:
	Context(bool isFuncTop, Context* parent);
	
	std::optional<Variable> getVariable(std::string varName);
	void defineLocal(std::string var, Type* type);
	void changeType(std::string var, Type* type);
	uint16_t getLocalCount();
	
	std::vector<int16_t>& getFunctionUpvalues();
	
private:
	bool isFuncTop;
	Context* parent;
	
	std::unordered_map<std::string, Variable> variables;
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
	
	GC::Root<TypeNamespace> types;
	Type *anyType, *nilType, *boolType, *realType, *intType, *stringType, *macroType;
	
	GC::Root<TypeNamespace> globals;
	
	Type* getType(Node& type);
	std::vector<int16_t> compileFunction(NodeBlock& block, std::vector<std::string> argNames, std::vector<Type*> argTypes, Type** resType, Context* parent = nullptr);
	void compileBlock(FunctionChunk& curFunc, NodeBlock& block, Context& ctx, Type** resType, bool mainBlock = false);
	void compileStatement(FunctionChunk& curFunc, Node& stat, Context& ctx, Type** resType);
	Type* compileExpression(FunctionChunk& curFunc, NodeExp& expr, Context& ctx);
	void compileConstant(FunctionChunk& curFunc, Value val);
};
