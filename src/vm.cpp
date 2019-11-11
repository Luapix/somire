#include "vm.hpp"

#include <iostream>
#include <string>

VM::VM() : stack(new List()), localBase(0), localCnt(0) {}

void VM::run(Chunk& chunk) {
	uint32_t pc = 0;
	while(pc < chunk.bytecode.size()) {
		Opcode op = static_cast<Opcode>(chunk.bytecode[pc]);
		switch(op) {
		case Opcode::IGNORE:
			stack->vec.resize(localBase + localCnt);
			pc++;
			break;
		case Opcode::CONSTANT: {
			uint8_t constantIdx = chunk.bytecode[pc+1];
			stack->vec.push_back(chunk.constants->vec.at(constantIdx));
			//std::cout << "Loaded constant n°" << (int) constantIdx << " = " << stack->vec.back().toString() << std::endl;
			pc += 2;
			break;
		} case Opcode::UNI_MINUS: {
			Value val = pop();
			stack->vec.push_back(val.negate());
			//std::cout << "Negated stack top; now equal to " << stack->vec.back().toString() << std::endl;
			pc++;
			break;
		} case Opcode::BIN_PLUS: {
			Value right = pop();
			Value left = pop();
			stack->vec.push_back(left.plus(right));
			//std::cout << "Added top two stack values; top now equal to " << stack->vec.back().toString() << std::endl;
			pc++;
			break;
		} case Opcode::LET: {
			localCnt++;
			//std::cout << "Set local n°" << localCnt-1 << " to " << stack->vec.back().toString() << std::endl;
			pc++;
			break;
		} case Opcode::SET: {
			uint8_t localIdx = chunk.bytecode[pc+1];
			if(localIdx >= localCnt)
				throw ExecutionError("Trying to assign to undefined local");
			Value val = pop();
			stack->vec[localBase + localIdx] = val;
			//std::cout << "Assigned " << stack->vec.back().toString() << " to local n°" << (int) localIdx << std::endl;
			pc += 2;
			break;
		} case Opcode::LOCAL: {
			uint8_t localIdx = chunk.bytecode[pc+1];
			if(localIdx >= localCnt)
				throw ExecutionError("Trying to access undefined local");
			stack->vec.push_back(stack->vec[localBase + localIdx]);
			//std::cout << "Pushed local n°" << (int) localIdx << " on the stack; top now equal to " << stack->vec.back().toString() << std::endl;
			pc += 2;
			break;
		} case Opcode::LOG:
			std::cout << "[log] " << pop().toString() << std::endl;
			pc++;
			break;
		default:
			throw ExecutionError("Opcode " + opcodeDesc(op) + " not yet implemented");
		}
		GC::collect();
		GC::logState();
	}
}

Value VM::pop() {
	if(stack->vec.empty())
		throw ExecutionError("Expected operand, stack empty");
	Value val = stack->vec.back();
	stack->vec.pop_back();
	return val;
}
