#pragma once

#include "chunk.hpp"
#include "value.hpp"

class VM {
public:
	VM();
	
	void run(Chunk& chunk);
};
