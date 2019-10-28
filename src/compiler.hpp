#pragma once

#include <memory>

#include "ast.hpp"

class Compiler {
public:
	Compiler(std::unique_ptr<Node> program);
	
	template <typename O>
	void writeProgram(O& output);
	
private:
	std::unique_ptr<Node> program;
};

#include "compiler.tpp"
