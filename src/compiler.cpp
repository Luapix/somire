#include "compiler.hpp"

Compiler::Compiler() {}

std::unique_ptr<Chunk> Compiler::compileChunk(std::unique_ptr<Node> ast) {
	return std::unique_ptr<Chunk>(new Chunk());
}
