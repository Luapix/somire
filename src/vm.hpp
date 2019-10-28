#pragma once

#include "chunk.hpp"

class VM {
public:
	VM();
	
	void run(Chunk& chunk);
};
