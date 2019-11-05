#pragma once

#include <stack>

#include "chunk.hpp"
#include "value.hpp"

class VM {
public:
	VM();
	
	void run(Chunk& chunk);
	
private:
	std::stack<std::unique_ptr<Value>> stack;
};
