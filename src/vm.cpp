#include "vm.hpp"

#include <iostream>
#include <string>

VM::VM() {}

void VM::run(Chunk& chunk) {
	uint32_t pc = 0;
	while(pc < chunk.bytecode.size()) {
		Opcode op = static_cast<Opcode>(chunk.bytecode[pc]);
		switch(op) {
		case Opcode::NO_OP:
			pc++;
			break;
		default:
			throw ExecutionError("Opcode " + std::to_string((int) op) + " not yet implemented");
		}
	}
}
