#include "compiler.hpp"

Compiler::Compiler(std::unique_ptr<Node> program) : program(std::move(program)) {}
