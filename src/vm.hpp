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
	GC::GCVector<Value>* stack;
	
	Value* pop();
};
