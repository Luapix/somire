#include "vm.hpp"

#include <iostream>
#include <string>

VM::VM() : stack(new GC::GCVector<Value>()) {
	GC::pin(stack);
}

VM::~VM() {
	GC::unpin(stack);
}

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
			stack->vec.push_back(chunk.constants->vec.at(constantIdx)); // Constants can only be used once for now...
			std::cout << "Loaded constant n°" << (int) constantIdx << " = " << stack->vec.back()->toString() << std::endl;
			pc += 2;
			break;
		} default:
			throw ExecutionError("Opcode " + std::to_string((int) op) + " not yet implemented");
		}
	}
}
