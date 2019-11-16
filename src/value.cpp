#include "value.hpp"

#include <string>
#include <sstream>

#include "fpconv.hpp"
#include "util.hpp"

ExecutionError::ExecutionError(const std::string& what)
	: runtime_error("Execution error: " + what) { }

std::string valueTypeDesc(ValueType type) {
	switch(type) {
	case ValueType::NIL: return "nil";
	case ValueType::BOOL: return "boolean";
	case ValueType::INT: return "int";
	case ValueType::REAL: return "real";
	case ValueType::LIST: return "list";
	case ValueType::STR: return "string";
	case ValueType::INTERNAL: return "internal";
	case ValueType::C_FUNC: return "C function";
	case ValueType::FUNC: return "function";
	default:
		throw std::runtime_error("Unknown type");
	}
}


Value::Value() : asBits(NIL) {}

Value::Value(bool boolean) : asBits(BOOL_TAG | (int) boolean) {}

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


void Value::mark() {
	if(isPointer()) getPointer()->mark();
}

ValueType Value::type() {
	if(isNil()) return ValueType::NIL;
	else if(isBool()) return ValueType::BOOL;
	else if(isInt()) return ValueType::INT;
	else if(isReal()) return ValueType::REAL;
	else if(isPointer()) return getPointer()->type;
	else throw std::runtime_error("Invalid value");
}

Value Value::negate() {
	if(isInt()) return Value(-getInt());
	else if(isReal()) return Value(-asDouble);
	else if(isPointer()) return getPointer()->negate();
	else throw ExecutionError("Cannot negate " + valueTypeDesc(type()));
}

Value Value::plus(Value other) {
	if(isInt() && other.isInt()) {
		return Value(getInt() + other.getInt());
	} else if(isNumeric() && other.isNumeric()) {
		return Value(convertToDouble() + other.convertToDouble());
	} else if(isPointer()) {
		return getPointer()->plus(other);
	} else {
		throw ExecutionError("Cannot add " + valueTypeDesc(type()) + " to " + valueTypeDesc(other.type()));
	}
}

Value Value::minus(Value other) {
	if(isInt() && other.isInt()) {
		return Value(getInt() - other.getInt());
	} else if(isNumeric() && other.isNumeric()) {
		return Value(convertToDouble() - other.convertToDouble());
	} else {
		throw ExecutionError("Cannot substract " + valueTypeDesc(type()) + " from " + valueTypeDesc(other.type()));
	}
}

Value Value::divide(Value other) {
	if(isNumeric() && other.isNumeric()) {
		double x2 = other.convertToDouble();
		if(x2 == 0.0) throw ExecutionError("Cannot divide by zero");
		return Value(convertToDouble() / x2);
	} else {
		throw ExecutionError("Cannot divide " + valueTypeDesc(type()) + " by " + valueTypeDesc(other.type()));
	}
}

Value Value::multiply(Value other) {
	if(isInt() && other.isInt()) {
		return Value(getInt() * other.getInt());
	} else if(isNumeric() && other.isNumeric()) {
		return Value(convertToDouble() * other.convertToDouble());
	} else {
		throw ExecutionError("Cannot multiply " + valueTypeDesc(type()) + " by " + valueTypeDesc(other.type()));
	}
}

bool Value::equals(Value other) {
	if(isInt() && other.isInt()) return getInt() == other.getInt();
	else if(isNumeric() && other.isNumeric()) return convertToDouble() == other.convertToDouble();
	else if(type() != other.type()) return false;
	else if(isNil()) return true;
	else if(isBool()) return getBool() == other.getBool();
	else if(isPointer()) return getPointer()->equals(*other.getPointer());
	else throw std::runtime_error("Invalid value");
}

bool Value::less(Value other) {
	if(isInt() && other.isInt()) return getInt() < other.getInt();
	else if(isNumeric() && other.isNumeric()) return convertToDouble() < other.convertToDouble();
	else throw ExecutionError("Cannot test if " + valueTypeDesc(type()) + " is less than " + valueTypeDesc(other.type()));
}

bool Value::less_or_eq(Value other) {
	if(isInt() && other.isInt()) return getInt() <= other.getInt();
	else if(isNumeric() && other.isNumeric()) return convertToDouble() <= other.convertToDouble();
	else throw ExecutionError("Cannot test if " + valueTypeDesc(type()) + " is less than or equal to " + valueTypeDesc(other.type()));
}

std::string Value::toString() {
	if(isNil()) {
		return "nil";
	} else if(isBool()) {
		return getBool() ? "true" : "false";
	} else if(isInt()) {
		return std::to_string(getInt());
	} else if(isReal()) {
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
	throw ExecutionError("Can't negate " + valueTypeDesc(type));
}

Value Object::plus(Value other) {
	throw ExecutionError("Cannot add " + valueTypeDesc(type) + " to " + valueTypeDesc(other.type()));
}

bool Object::equals(Object& obj) {
	return this == &obj;
}

std::string Object::toString() {
	std::stringstream ss;
	ss << "<" << valueTypeDesc(type) << " " << this << ">";
	return ss.str();
}


Namespace::Namespace() : Object(ValueType::INTERNAL) {}

void Namespace::markChildren() {
	for(auto& pair : map) {
		pair.second.mark();
	}
}


List::List() : Object(ValueType::LIST) {}

void List::markChildren() {
	for(Value val : vec) {
		val.mark();
	}
}


String::String(std::string str) : Object(ValueType::STR), str(str) {}

bool String::equals(Object& obj) {
	return str == static_cast<String&>(obj).str;
}

std::string String::toString() {
	return escapeString(str);
}


CFunction::CFunction(std::function<Value(std::vector<Value>&)> func) : Object(ValueType::C_FUNC), func(func) {}

Function::Function(uint16_t protoIdx, uint16_t argCnt) : Object(ValueType::FUNC), protoIdx(protoIdx), argCnt(argCnt) {}
