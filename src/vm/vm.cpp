#include "vm.hpp"

#include <iostream>
#include <string>


Stack::Stack() : base(&array[0]), top((Value*) base) {}

std::vector<Value> Stack::popN(uint32_t n) {
	if(size() < n) throw ExecutionError("Stack is too small to pop " + std::to_string(n) + " values");
	std::vector<Value> res(top - n, top);
	top -= n;
	return res;
}

void Stack::removeN(uint32_t n) {
	if(size() < n) throw ExecutionError("Stack is too small to remove " + std::to_string(n) + " values");
	top -= n;
}

void Stack::markChildren() {
	for(auto it = &array[0]; it != top; it++) {
		it->mark();
	}
}


VM::VM() : globals(new Namespace()), stack(new Stack()) {
	loadStd(*globals);
}

void VM::run(Chunk& chunk) {
	calls.emplace_back(new ExecutionRecord(0, 0));
	
	uint32_t funcIdx = 0;
	auto it = chunk.functions[0]->code.begin();
	bool returnNow = false;
	while(calls.size() > 0) {
		Opcode op = (Opcode) readUI8(it);
		switch(op) {
		case Opcode::IGNORE:
			stack->pop();
			break;
		case Opcode::CONSTANT: {
			uint16_t constantIdx = readUI16(it);
			stack->push(chunk.constants->vec.at(constantIdx));
			break;
		} case Opcode::UNI_MINUS: {
			Value val = stack->pop();
			stack->push(val.negate());
			break;
		} case Opcode::BIN_PLUS: {
			Value right = stack->pop();
			Value left = stack->pop();
			stack->push(left.plus(right));
			break;
		} case Opcode::BIN_MINUS: {
			Value right = stack->pop();
			Value left = stack->pop();
			stack->push(left.minus(right));
			break;
		} case Opcode::MULTIPLY: {
			Value right = stack->pop();
			Value left = stack->pop();
			stack->push(left.multiply(right));
			break;
		} case Opcode::DIVIDE: {
			Value right = stack->pop();
			Value left = stack->pop();
			stack->push(left.divide(right));
			break;
		} case Opcode::MODULO: {
			Value right = stack->pop();
			Value left = stack->pop();
			stack->push(left.modulo(right));
			break;
		} case Opcode::POWER: {
			Value right = stack->pop();
			Value left = stack->pop();
			stack->push(left.power(right));
			break;
		} case Opcode::NOT: {
			Value val = stack->pop();
			if(!val.isBool()) throw ExecutionError("Cannot 'not' non-boolean value " + val.toString());
			stack->push(Value(!val.getBool()));
			break;
		} case Opcode::AND: {
			Value right = stack->pop();
			Value left = stack->pop();
			if(!left.isBool() || !right.isBool()) throw ExecutionError("Cannot 'and' " + left.toString() + " and " + right.toString());
			stack->push(Value(left.getBool() && right.getBool()));
			break;
		} case Opcode::OR: {
			Value right = stack->pop();
			Value left = stack->pop();
			if(!left.isBool() || !right.isBool()) throw ExecutionError("Cannot 'or' " + left.toString() + " and " + right.toString());
			stack->push(Value(left.getBool() || right.getBool()));
			break;
		} case Opcode::EQUALS: {
			Value right = stack->pop();
			Value left = stack->pop();
			stack->push(Value(left.equals(right)));
			break;
		} case Opcode::LESS: {
			Value right = stack->pop();
			Value left = stack->pop();
			stack->push(Value(left.less(right)));
			break;
		} case Opcode::LESS_OR_EQ: {
			Value right = stack->pop();
			Value left = stack->pop();
			stack->push(Value(left.less_or_eq(right)));
			break;
		} case Opcode::LET: {
			calls.back()->localCnt++;
			break;
		} case Opcode::POP: {
			uint16_t amount = readUI16(it);
			popLocals(amount);
			break;
		} case Opcode::SET_LOCAL: {
			int16_t localIdx = readI16(it);
			if(localIdx >= 0) {
				getLocal(localIdx) = stack->pop();
			} else {
				getUpvalue(localIdx).resolve() = stack->pop();
			}
			break;
		} case Opcode::LOCAL: {
			int16_t localIdx = readI16(it);
			if(localIdx >= 0) {
				stack->push(getLocal(localIdx));
			} else {
				stack->push(getUpvalue(localIdx).resolve());
			}
			break;
		} case Opcode::GLOBAL: {
			uint16_t constantIdx = readUI16(it);
			Value nameValue = chunk.constants->vec.at(constantIdx);
			String* nameStr = nameValue.get<String>();
			if(!nameStr) throw ExecutionError("Tring to access global by name " + nameValue.toString());
			std::string name = nameStr->str;
			auto it = globals->map.find(name);
			if(it == globals->map.end()) throw ExecutionError("Tring to access undefined global " + name);
			stack->push(it->second);
			break;
		} case Opcode::JUMP_IF_NOT: {
			Value cond = stack->pop();
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
			
			Value funcValue = stack->pop();
			
			CFunction* cfunc;
			Function* func;
			if(cfunc = funcValue.get<CFunction>()) {
				// Pop the arguments off the stack
				std::vector<Value> args = stack->popN(argCnt);
				Value res = cfunc->func(args);
				stack->push(res);
			} else if(func = funcValue.get<Function>()) {
				// We leave the arguments on the stack, they will become locals
				if(argCnt != func->argCnt)
					throw ExecutionError("Expected " + std::to_string(func->argCnt) + " arguments, got " + std::to_string(argCnt));
				
				calls.back()->funcIdx = funcIdx;
				calls.back()->codeOffset = it - chunk.functions[funcIdx]->code.begin();
				
				funcIdx = func->protoIdx;
				calls.emplace_back(new ExecutionRecord(stack->size() - argCnt, argCnt, func));
				it = chunk.functions[funcIdx]->code.begin();
			} else {
				throw ExecutionError("Cannot call " + funcValue.getTypeDesc());
			}
			break;
		} case Opcode::RETURN: {
			Value val = stack->pop();
			popLocals(calls.back()->localCnt);
			stack->push(val); // Push return value
			returnNow = true;
			break;
		} case Opcode::MAKE_FUNC: {
			uint16_t protoIdx = readUI16(it);
			uint16_t argCnt = readUI16(it);
			uint16_t upvalueCnt = readUI16(it);
			Function* func = new Function(protoIdx, argCnt, upvalueCnt);
			for(uint16_t i = 0; i < upvalueCnt; i++) {
				int16_t idx = readI16(it);
				ExecutionRecord& record = *calls.back();
				if(idx >= 0) {
					auto it = record.upvalueBackPointers.find(idx);
					if(it != record.upvalueBackPointers.end()) {
						func->upvalues[i] = it->second;
					} else {
						Value* value;
						if(idx == record.localCnt) { // recursive call (hopefully)
							value = &stack->array[record.localBase + idx];
						} else {
							value = &getLocal(idx);
						}
						Upvalue* upvalue = new Upvalue(value, &record, idx);
						record.upvalueBackPointers[idx] = upvalue;
						func->upvalues[i] = upvalue;
					}
				} else {
					func->upvalues[i] = &getUpvalue(idx);
				}
			}
			stack->push(Value(func));
			break;
		} case Opcode::MAKE_LIST: {
			uint16_t valueCnt = readUI16(it);
			std::vector<Value> vals = stack->popN(valueCnt);
			stack->push(Value(new List(std::move(vals))));
			break;
		} case Opcode::INDEX: {
			Value index = stack->pop();
			Value listValue = stack->pop();
			List* list = listValue.get<List>();
			if(!list)
				throw ExecutionError("Cannot index " + listValue.getTypeDesc());
			if(!index.isInt())
				throw ExecutionError("Cannot index list with " + index.getTypeDesc());
			int32_t index2 = index.getInt();
			if(index2 < 1 || index2 > list->vec.size())
				throw ExecutionError("List index out of range: " + std::to_string(index2));
			stack->push(list->vec[index2-1]);
			break;
		} default:
			throw ExecutionError("Opcode " + opcodeDesc(op) + " not yet implemented");
		}
		
		if(!returnNow && it == chunk.functions[funcIdx]->code.end()) { // implicit "return nil"
			popLocals(calls.back()->localCnt);
			stack->push(Value::nil());
			returnNow = true;
		}
		
		if(returnNow) {
			returnNow = false;
			uint32_t leftOnStack = stack->size() - calls.back()->localBase;
			if(leftOnStack != 1)
				throw ExecutionError("Unexpected number of values on stack at the end of function: " + std::to_string(leftOnStack));
			calls.pop_back();
			
			if(calls.size() == 0) { // we just exited the main function
				break;
			} else {
				funcIdx = calls.back()->funcIdx;
				it = chunk.functions[funcIdx]->code.begin() + calls.back()->codeOffset;
			}
		}
		
		GC::step();
	}
	
	GC::collect();
}

inline Value& VM::getLocal(uint16_t idx) {
	if(idx >= calls.back()->localCnt)
		throw ExecutionError("Trying to access undefined local");
	return stack->array[calls.back()->localBase + idx];
}

inline Upvalue& VM::getUpvalue(int16_t idx) {
	if(idx >= 0) throw ExecutionError("Trying to access invalid upvalue");
	Function* func = calls.back()->func;
	if(!func) throw ExecutionError("Cannot access upvalues in main chunk");
	return *func->upvalues[-idx-1];
}

void VM::popLocals(uint16_t amount) {
	ExecutionRecord& record = *calls.back();
	for(uint16_t i = record.localCnt - amount; i < record.localCnt; i++) {
		auto it = record.upvalueBackPointers.find(i);
		if(it != record.upvalueBackPointers.end()) {
			it->second->close();
		}
	}
	calls.back()->localCnt -= amount;
	stack->removeN(amount);
}
