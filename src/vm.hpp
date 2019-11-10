#pragma once

#include "chunk.hpp"
#include "value.hpp"
#include "gc.hpp"

class VM {
public:
	VM();
	
	void run(Chunk& chunk);
	
private:
	GC::Root<List> stack;
	
	Value pop();
	void clearStack();
};
