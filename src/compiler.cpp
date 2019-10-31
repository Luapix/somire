#include "compiler.hpp"

CompileError::CompileError(const std::string& what)
	: runtime_error("Compile error: " + what) { }

Compiler::Compiler() {}

std::unique_ptr<Chunk> Compiler::compileChunk(std::unique_ptr<Node> ast) {
	std::unique_ptr<Chunk> chunk(new Chunk());
	if(ast->type != NodeType::BLOCK)
		throw CompileError("Expected block to compile, got " + nodeTypeDesc(ast->type));
	compileBlock(*chunk, static_cast<NodeBlock&>(*ast));
	return chunk;
}

void Compiler::compileBlock(Chunk& chunk, NodeBlock& block) {
	chunk.constants.push_back(std::unique_ptr<Value>(new Value(ValueType::NIL)));
	chunk.constants.push_back(std::unique_ptr<Value>(new ValueInt(42)));
	
	chunk.bytecode.push_back((uint8_t) Opcode::CONSTANT);
	chunk.bytecode.push_back((uint8_t) 0);
	chunk.bytecode.push_back((uint8_t) Opcode::CONSTANT);
	chunk.bytecode.push_back((uint8_t) 1);
}
