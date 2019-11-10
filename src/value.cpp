#include "value.hpp"

#include <string>
#include <sstream>

#include "fpconv.hpp"

ExecutionError::ExecutionError(const std::string& what)
	: runtime_error("Execution error: " + what) { }

std::string valueTypeDesc(ValueType type) {
	switch(type) {
	case ValueType::NIL: return "nil";
	case ValueType::INT: return "int";
	case ValueType::REAL: return "real";
	case ValueType::LIST: return "list";
	default:
		throw std::runtime_error("Unknown type");
	}
}


Value::Value(int32_t integer) {
	asBits = INT_TAG | (uint32_t) integer;
}

Value::Value(double real) {
	if(real > POINTER_TAG)
		throw std::runtime_error("No payload is allowed in quiet NaNs");
	asDouble = real;
}

Value::Value(Object* obj) {
	uint64_t add = reinterpret_cast<uint64_t>(obj);
	if(add == 0)
		throw std::runtime_error("Trying to store null object");
	if(add > 0xffffffffffff)
		throw std::runtime_error("Can only store 48-bit pointers to objects");
	asBits = POINTER_TAG | add;
}

Value Value::nil() {
	Value v;
	v.asBits = NIL;
	return v;
}


void Value::mark() {
	if(isPointer()) getPointer()->mark();
}

ValueType Value::type() {
	if(isNil()) return ValueType::NIL;
	else if(isInt()) return ValueType::INT;
	else if(isDouble()) return ValueType::REAL;
	else if(isPointer()) return getPointer()->type;
	else throw std::runtime_error("Invalid value");
}

bool Value::isNil() { return asBits == NIL; }
bool Value::isInt() { return INT_TAG <= asBits; }
bool Value::isDouble() { return asBits <= POINTER_TAG; }
bool Value::isPointer() { return POINTER_TAG < asBits && asBits < INT_TAG; }

int32_t Value::getInt() { return (int32_t) ((uint32_t) asBits); }
double Value::getReal() { return asDouble; }
Object* Value::getPointer() { return reinterpret_cast<Object*>(asBits & 0xffffffffffff); }

Value Value::negate() {
	if(isNil()) throw ExecutionError("Cannot negate nil value");
	else if(isInt()) return Value(-getInt());
	else if(isDouble()) return Value(-asDouble);
	else if(isPointer()) return getPointer()->negate();
	else throw std::runtime_error("Invalid value");
}

Value Value::plus(Value other) {
	if(isNil()) {
		throw ExecutionError("Cannot add nil value to anything");
	} else if(isInt()) {
		if(other.isInt())
			return Value(getInt() + other.getInt());
		else
			throw ExecutionError("Cannot add int to " + valueTypeDesc(other.type()) + " value");
	} else if(isDouble()) {
		if(other.isDouble())
			return Value(asDouble + other.asDouble);
		else
			throw ExecutionError("Cannot add real to " + valueTypeDesc(other.type()) + " value");
	} else if(isPointer()) {
		return getPointer()->plus(other);
	} else {
		throw std::runtime_error("Invalid value");
	}
}

std::string Value::toString() {
	if(isNil()) {
		return "nil";
	} else if(isInt()) {
		return std::to_string(getInt());
	} else if(isDouble()) {
		char buf[24];
		return std::string(buf, fpconv_dtoa(asDouble, buf));
	} else if(isPointer()) {
		return getPointer()->toString();
	} else {
		throw std::runtime_error("Invalid value");
	}
}


Object::Object(ValueType type) : type(type) {}

Value Object::negate() {
	throw ExecutionError("Can't negate " + valueTypeDesc(type) + " value");
}

Value Object::plus(Value other) {
	throw ExecutionError("Cannot add " + valueTypeDesc(type) + " value to anything");
}

std::string Object::toString() {
	std::stringstream ss;
	ss << "<" << valueTypeDesc(type) << " " << this << ">";
	return ss.str();
}


List::List() : Object(ValueType::LIST) {}

void List::markChildren() {
	for(Value val : vec) {
		val.mark();
	}
}
