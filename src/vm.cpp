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
		case Opcode::IGNORE:
			clearStack();
			pc++;
			break;
		case Opcode::CONSTANT: {
			uint8_t constantIdx = chunk.bytecode[pc+1];
			stack->vec.push_back(chunk.constants->vec.at(constantIdx)); // Constants can only be used once for now...
			std::cout << "Loaded constant n°" << (int) constantIdx << " = " << stack->vec.back()->toString() << std::endl;
			pc += 2;
			break;
		} case Opcode::UNI_MINUS: {
			GC::GCRoot<Value> val(pop());
			stack->vec.push_back(val->negate());
			std::cout << "Negated stack top; now equal to " << stack->vec.back()->toString() << std::endl;
			pc++;
			break;
		} case Opcode::BIN_PLUS: {
			GC::GCRoot<Value> right(pop());
			GC::GCRoot<Value> left(pop());
			stack->vec.push_back(left->plus(*right));
			std::cout << "Added top two stack values; top now equal to " << stack->vec.back()->toString() << std::endl;
			pc++;
			break;
		} default:
			throw ExecutionError("Opcode " + opcodeDesc(op) + " not yet implemented");
		}
		GC::collect();
		GC::logState();
	}
}

Value* VM::pop() {
	if(stack->vec.empty())
		throw ExecutionError("Expected operand, stack empty");
	Value* val = stack->vec.back();
	stack->vec.pop_back();
	return val;
}

void VM::clearStack() {
	stack->vec.clear();
}
