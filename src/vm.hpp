#pragma once

#include "chunk.hpp"

class ExecutionError : public std::runtime_error {
public:
	ExecutionError(const std::string& what);
};

class VM {
public:
	VM();
	
	void run(Chunk& chunk);
};
