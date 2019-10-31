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
		case Opcode::CONSTANT: {
			uint8_t constantIdx = chunk.bytecode[pc+1];
			Value& val = *chunk.constants.at(constantIdx);
			std::cout << "Constant n°" << (int) constantIdx << " = " << val.toString() << std::endl;
			pc += 2;
			break;
		} default:
			throw ExecutionError("Opcode " + std::to_string((int) op) + " not yet implemented");
		}
	}
}
