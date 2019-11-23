#pragma once

#include "compiler/chunk.hpp"
#include "value.hpp"
#include "gc.hpp"
#include "std.hpp"


// For upvalues to work efficiently, the stack should not be reallocated, hence:
const uint32_t STACK_SIZE = 0xffff;

class Stack : public Object {
public:
	std::array<Value, STACK_SIZE> array;
	const Value* base;
	Value* top;
	
	Stack();
	
	inline Value* begin() { return (Value*) base; }
	inline Value* end() { return top; }
	
	inline void push(Value val) {
		*(top++) = val;
		if(size() >= STACK_SIZE) throw ExecutionError("Stack overflow");
	}
	inline Value pop() {
		if(top == base) throw ExecutionError("Stack is empty, cannot pop");
		return *(--top);
	}
	inline uint32_t size() { return top - base; }
	
	std::vector<Value> popN(uint32_t n);
	void removeN(uint32_t n);
	
	void markChildren() override;
};

class VM {
public:
	VM();
	
	void run(Chunk& chunk);
	
private:
	GC::Root<Namespace> globals;
	GC::Root<Stack> stack;
	std::vector<std::unique_ptr<ExecutionRecord>> calls;
	
	Value pop();
	
	Value& getLocal(uint16_t idx);
	Upvalue& getUpvalue(int16_t idx);
	void popLocals(uint16_t amount);
};
