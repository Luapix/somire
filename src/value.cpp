#include "value.hpp"

#include <string>

ExecutionError::ExecutionError(const std::string& what)
	: runtime_error("Execution error: " + what) { }

Value::Value(ValueType type) : type(type) {}

std::string Value::toString() {
	switch(type) {
	case ValueType::NIL:
		return "nil";
	default:
		throw ExecutionError("String representation not implemented for this type");
	}
}

ValueInt::ValueInt(int32_t val) : Value(ValueType::INT), val(val) {}

ValueInt* ValueInt::negate() {
	return new ValueInt(-val);
}

std::string ValueInt::toString() {
	return std::to_string(val);
}
