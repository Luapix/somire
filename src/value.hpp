#pragma once

#include <stdexcept>
#include <cstdint>

#include "gc.hpp"

class ExecutionError : public std::runtime_error {
public:
	ExecutionError(const std::string& what);
};

enum class ValueType : uint8_t {
	NIL,
	INT
};

std::string valueTypeDesc(ValueType type);

class Value : public GC::GCObject {
public:
	const ValueType type;
	
	Value(ValueType type);
	virtual ~Value() = default;
	
	virtual Value* negate();
	virtual Value* plus(Value& other);
	
	virtual std::string toString();
};

class ValueInt : public Value {
public:
	const int32_t val;
	
	ValueInt(int32_t val);
	
	ValueInt* negate() override;
	ValueInt* plus(Value& other) override;
	
	std::string toString() override;
};
