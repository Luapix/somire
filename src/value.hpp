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
	INT,
	REAL
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
	
	Value* negate() override;
	Value* plus(Value& other) override;
	
	std::string toString() override;
};

class ValueReal : public Value {
public:
	const double val;
	
	ValueReal(double val);
	
	Value* negate() override;
	Value* plus(Value& other) override;
	
	std::string toString() override;
};
