#pragma once

#include "chunk.hpp"
#include "value.hpp"
#include "gc.hpp"
#include "std.hpp"


struct ExecutionRecord {
	uint32_t localBase;
	uint32_t localCnt;
	
	uint32_t funcIdx;
	uint32_t codeOffset;
	
	ExecutionRecord(uint32_t localBase, uint32_t localCnt);
};

class VM {
public:
	VM();
	
	void run(Chunk& chunk);
	
private:
	GC::Root<Namespace> globals;
	GC::Root<List> stack;
	std::vector<ExecutionRecord> calls;
	
	Value pop();
};
