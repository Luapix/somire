#pragma once

#include <stdexcept>
#include <cstdint>

class ExecutionError : public std::runtime_error {
public:
	ExecutionError(const std::string& what);
};

enum class ValueType : uint8_t {
	NIL,
	INT
};

class Value {
public:
	const ValueType type;
	
	Value(ValueType type);
	virtual ~Value() = default;
	
	virtual std::string toString();
};

class ValueInt : public Value {
public:
	const int32_t val;
	
	ValueInt(int32_t val);
	
	std::string toString() override;
};
