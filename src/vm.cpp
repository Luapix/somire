#include "vm.hpp"

#include <iostream>
#include <string>
#include <algorithm>

VM::VM() : globals(new Namespace()), stack(new List()), localBase(0), localCnt(0) {
	loadStd(*globals);
}

void VM::run(Chunk& chunk) {
	auto it = chunk.bytecode.begin();
	while(it != chunk.bytecode.end()) {
		Opcode op = (Opcode) readUI8(it);
		switch(op) {
		case Opcode::IGNORE:
			stack->vec.resize(localBase + localCnt);
			break;
		case Opcode::CONSTANT: {
			uint16_t constantIdx = readUI16(it);
			stack->vec.push_back(chunk.constants->vec.at(constantIdx));
			//std::cout << "Loaded constant n°" << (int) constantIdx << " = " << stack->vec.back().toString() << std::endl;
			break;
		} case Opcode::UNI_MINUS: {
			Value val = pop();
			stack->vec.push_back(val.negate());
			//std::cout << "Negated stack top; now equal to " << stack->vec.back().toString() << std::endl;
			break;
		} case Opcode::BIN_PLUS: {
			Value right = pop();
			Value left = pop();
			stack->vec.push_back(left.plus(right));
			//std::cout << "Added top two stack values; top now equal to " << stack->vec.back().toString() << std::endl;
			break;
		} case Opcode::BIN_MINUS: {
			Value right = pop();
			Value left = pop();
			stack->vec.push_back(left.minus(right));
			break;
		} case Opcode::MULTIPLY: {
			Value right = pop();
			Value left = pop();
			stack->vec.push_back(left.multiply(right));
			break;
		} case Opcode::DIVIDE: {
			Value right = pop();
			Value left = pop();
			stack->vec.push_back(left.divide(right));
			//std::cout << "Added top two stack values; top now equal to " << stack->vec.back().toString() << std::endl;
			break;
		} case Opcode::NOT: {
			Value val = pop();
			if(!val.isBool()) throw ExecutionError("Cannot 'not' non-boolean value " + val.toString());
			stack->vec.emplace_back(!val.getBool());
			break;
		} case Opcode::AND: {
			Value right = pop();
			Value left = pop();
			if(!left.isBool() || !right.isBool()) throw ExecutionError("Cannot 'and' " + left.toString() + " and " + right.toString());
			stack->vec.emplace_back(left.getBool() && right.getBool());
			break;
		} case Opcode::OR: {
			Value right = pop();
			Value left = pop();
			if(!left.isBool() || !right.isBool()) throw ExecutionError("Cannot 'or' " + left.toString() + " and " + right.toString());
			stack->vec.emplace_back(left.getBool() || right.getBool());
			break;
		} case Opcode::EQUALS: {
			Value right = pop();
			Value left = pop();
			stack->vec.emplace_back(left.equals(right));
			break;
		} case Opcode::LET: {
			localCnt++;
			//std::cout << "Set local n°" << localCnt-1 << " to " << stack->vec.back().toString() << std::endl;
			break;
		} case Opcode::POP: {
			uint16_t amount = readUI16(it);
			localCnt -= amount;
			stack->vec.resize(localBase + localCnt);
			break;
		} case Opcode::SET: {
			uint16_t localIdx = readUI16(it);
			if(localIdx >= localCnt)
				throw ExecutionError("Trying to assign to undefined local");
			Value val = pop();
			stack->vec[localBase + localIdx] = val;
			//std::cout << "Assigned " << stack->vec.back().toString() << " to local n°" << (int) localIdx << std::endl;
			break;
		} case Opcode::LOCAL: {
			uint16_t localIdx = readUI16(it);
			if(localIdx >= localCnt)
				throw ExecutionError("Trying to access undefined local");
			stack->vec.push_back(stack->vec[localBase + localIdx]);
			//std::cout << "Pushed local n°" << (int) localIdx << " on the stack; top now equal to " << stack->vec.back().toString() << std::endl;
			break;
		} case Opcode::GLOBAL: {
			uint16_t constantIdx = readUI16(it);
			Value nameValue = chunk.constants->vec.at(constantIdx);
			if(nameValue.type() != ValueType::STR) throw ExecutionError("Tring to access global by name " + nameValue.toString());
			std::string name = static_cast<String*>(nameValue.getPointer())->str;
			auto it = globals->map.find(name);
			if(it == globals->map.end()) throw ExecutionError("Tring to access undefined global " + name);
			stack->vec.push_back(it->second);
			break;
		} case Opcode::JUMP_IF_NOT: {
			Value cond = pop();
			int16_t relJump = readI16(it);
			if(!cond.isBool()) throw ExecutionError("Expected boolean in 'if' condition, got " + cond.toString());
			if(!cond.getBool())
				it += relJump;
			break;
		} case Opcode::JUMP:
			it += readI16(it);
			break;
		case Opcode::CALL: {
			uint16_t argCnt = readUI16(it);
			std::vector<Value> args;
			args.resize(argCnt);
			std::copy_n(stack->vec.rbegin(), argCnt, args.rbegin());
			stack->vec.resize(stack->vec.size() - argCnt);
			Value func = pop();
			stack->vec.push_back(func.call(args));
			break;
		} default:
			throw ExecutionError("Opcode " + opcodeDesc(op) + " not yet implemented");
		}
		GC::step();
	}
}

Value VM::pop() {
	if(stack->vec.empty())
		throw ExecutionError("Expected operand, stack empty");
	Value val = stack->vec.back();
	stack->vec.pop_back();
	return val;
}
