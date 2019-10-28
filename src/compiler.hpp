#pragma once

#include <memory>

#include "ast.hpp"
#include "chunk.hpp"

class Compiler {
public:
	Compiler();
	
	std::unique_ptr<Chunk> compileChunk(std::unique_ptr<Node> ast);
};
