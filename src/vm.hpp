#pragma once

#include "chunk.hpp"
#include "value.hpp"
#include "gc.hpp"

class VM {
public:
	VM();
	~VM();
	
	void run(Chunk& chunk);
	
private:
	List* stack;
	
	Value pop();
	void clearStack();
};
