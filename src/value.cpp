#include "value.hpp"

#include <string>
#include <sstream>

ExecutionError::ExecutionError(const std::string& what)
	: runtime_error("Execution error: " + what) { }

std::string valueTypeDesc(ValueType type) {
	switch(type) {
	case ValueType::NIL: return "nil";
	case ValueType::INT: return "int";
	case ValueType::REAL: return "real";
	default:
		throw std::runtime_error("Unknown type");
	}
}

Value::Value(ValueType type) : type(type) {}

Value* Value::negate() {
	throw ExecutionError("Can't negate " + valueTypeDesc(type) + " value");
}

Value* Value::plus(Value& other) {
	throw ExecutionError("Cannot add " + valueTypeDesc(type) + " value to anything");
}

std::string Value::toString() {
	switch(type) {
	case ValueType::NIL:
		return "nil";
	default:
		std::stringstream ss;
		ss << "<" << valueTypeDesc(type) << " " << this << ">";
		return ss.str();
	}
}

ValueInt::ValueInt(int32_t val) : Value(ValueType::INT), val(val) {}

Value* ValueInt::negate() {
	return new ValueInt(-val);
}

Value* ValueInt::plus(Value& other) {
	if(other.type != ValueType::INT)
		throw ExecutionError("Cannot add int to " + valueTypeDesc(other.type) + " value");
	return new ValueInt(val + static_cast<ValueInt&>(other).val);
}

std::string ValueInt::toString() {
	return std::to_string(val);
}

ValueReal::ValueReal(double val) : Value(ValueType::REAL), val(val) {}

Value* ValueReal::negate() {
	return new ValueReal(-val);
}

Value* ValueReal::plus(Value& other) {
	if(other.type != ValueType::REAL)
		throw ExecutionError("Cannot add real to " + valueTypeDesc(other.type) + " value");
	return new ValueReal(val + static_cast<ValueReal&>(other).val);
}

std::string ValueReal::toString() {
	return std::to_string(val);
}
