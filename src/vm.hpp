#pragma once

#include "chunk.hpp"
#include "value.hpp"
#include "gc.hpp"
#include "std.hpp"

class VM {
public:
	VM();
	
	void run(Chunk& chunk);
	
private:
	GC::Root<Namespace> globals;
	GC::Root<List> stack;
	uint32_t localBase;
	uint32_t localCnt;
	
	Value pop();
};
