#include "vm.hpp"

#include <iostream>
#include <string>
#include <algorithm>

ExecutionRecord::ExecutionRecord(uint32_t localBase, uint32_t localCnt) : localBase(localBase), localCnt(localCnt) {}

VM::VM() : globals(new Namespace()), stack(new List()) {
	loadStd(*globals);
}

void VM::run(Chunk& chunk) {
	calls.emplace_back(0, 0);
	runFunction(chunk, 0);
	calls.pop_back();
}

void VM::runFunction(Chunk& chunk, uint16_t funcIdx) {
	auto it = chunk.functions[funcIdx]->code.begin();
	while(it != chunk.functions[funcIdx]->code.end()) {
		Opcode op = (Opcode) readUI8(it);
		switch(op) {
		case Opcode::IGNORE:
			stack->vec.resize(calls.back().localBase + calls.back().localCnt);
			break;
		case Opcode::CONSTANT: {
			uint16_t constantIdx = readUI16(it);
			stack->vec.push_back(chunk.constants->vec.at(constantIdx));
			break;
		} case Opcode::UNI_MINUS: {
			Value val = pop();
			stack->vec.push_back(val.negate());
			break;
		} case Opcode::BIN_PLUS: {
			Value right = pop();
			Value left = pop();
			stack->vec.push_back(left.plus(right));
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
		} case Opcode::LESS: {
			Value right = pop();
			Value left = pop();
			stack->vec.emplace_back(left.less(right));
			break;
		} case Opcode::LESS_OR_EQ: {
			Value right = pop();
			Value left = pop();
			stack->vec.emplace_back(left.less_or_eq(right));
			break;
		} case Opcode::LET: {
			calls.back().localCnt++;
			break;
		} case Opcode::POP: {
			uint16_t amount = readUI16(it);
			calls.back().localCnt -= amount;
			stack->vec.resize(calls.back().localBase + calls.back().localCnt);
			break;
		} case Opcode::SET: {
			uint16_t localIdx = readUI16(it);
			if(localIdx >= calls.back().localCnt)
				throw ExecutionError("Trying to assign to undefined local");
			Value val = pop();
			stack->vec[calls.back().localBase + localIdx] = val;
			break;
		} case Opcode::LOCAL: {
			uint16_t localIdx = readUI16(it);
			if(localIdx >= calls.back().localCnt)
				throw ExecutionError("Trying to access undefined local");
			stack->vec.push_back(stack->vec[calls.back().localBase + localIdx]);
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
			
			Value func = pop();
			ValueType type = func.type();
			
			if(type == ValueType::C_FUNC) {
				// Pop the arguments off the stack
				std::vector<Value> args;
				args.resize(argCnt);
				std::copy_n(stack->vec.rbegin(), argCnt, args.rbegin());
				stack->vec.resize(stack->vec.size() - argCnt);
				
				Value res = static_cast<CFunction*>(func.getPointer())->func(args);
				stack->vec.push_back(res);
			} else if(type == ValueType::FUNC) {
				// We leave the arguments on the stack, they will become locals
				Function* func2 = static_cast<Function*>(func.getPointer());
				if(argCnt != func2->argCnt)
					throw ExecutionError("Expected " + std::to_string(func2->argCnt) + " arguments, got " + std::to_string(argCnt));
				calls.emplace_back(stack->vec.size() - argCnt, argCnt);
				runFunction(chunk, func2->protoIdx);
				calls.pop_back();
				stack->vec.emplace_back(); // For now, Somiré functions have no way of returning a value
			} else {
				throw ExecutionError("Cannot call " + valueTypeDesc(type));
			}
			break;
		} case Opcode::MAKE_FUNC:
			stack->vec.emplace_back(new Function(readUI16(it), readUI16(it)));
			break;
		default:
			throw ExecutionError("Opcode " + opcodeDesc(op) + " not yet implemented");
		}
		GC::step();
	}
}

inline Value VM::pop() {
	if(stack->vec.empty())
		throw ExecutionError("Expected operand, stack empty");
	Value val = stack->vec.back();
	stack->vec.pop_back();
	return val;
}
