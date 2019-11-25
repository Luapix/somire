#include "value.hpp"

#include <string>
#include <sstream>
#include <cmath>

#include "util/fpconv.hpp"
#include "util/util.hpp"

ExecutionError::ExecutionError(const std::string& what)
	: runtime_error("Execution error: " + what) { }


Value Value::nil() { return Value::fromBits((uint64_t) NIL); }
Value::Value(bool boolean) : asBits(BOOL_TAG | (uint32_t) boolean) {}
Value::Value(int32_t integer) : asBits(INT_TAG | (uint32_t) integer) {}

Value::Value(double real) {
	if(real > OBJECT_TAG)
		throw std::runtime_error("No payload is allowed in quiet NaNs");
	asDouble = real;
}

Value::Value(Object* obj) {
	uint64_t add = reinterpret_cast<uint64_t>(obj);
	if(add == 0)
		throw std::runtime_error("Trying to store null object");
	if(add > 0xffffffffffff)
		throw std::runtime_error("Can only store 48-bit pointers to objects");
	asBits = OBJECT_TAG | add;
}


void Value::mark() {
	if(isObject()) getObject()->mark();
}

Value Value::negate() {
	if(isInt()) return Value(-getInt());
	else if(isReal()) return Value(-asDouble);
	else if(isObject()) return getObject()->negate();
	else throw ExecutionError("Cannot negate " + getTypeDesc());
}

Value Value::plus(Value other) {
	if(isInt() && other.isInt()) {
		return Value(getInt() + other.getInt());
	} else if(isNumeric() && other.isNumeric()) {
		return Value(convertToDouble() + other.convertToDouble());
	} else if(isObject()) {
		return getObject()->plus(other);
	} else {
		throw ExecutionError("Cannot add " + getTypeDesc() + " to " + other.getTypeDesc());
	}
}

Value Value::minus(Value other) {
	if(isInt() && other.isInt()) {
		return Value(getInt() - other.getInt());
	} else if(isNumeric() && other.isNumeric()) {
		return Value(convertToDouble() - other.convertToDouble());
	} else {
		throw ExecutionError("Cannot substract " + getTypeDesc() + " from " + other.getTypeDesc());
	}
}

Value Value::divide(Value other) {
	if(isNumeric() && other.isNumeric()) {
		double x2 = other.convertToDouble();
		if(x2 == 0.0) throw ExecutionError("Cannot divide by zero");
		return Value(convertToDouble() / x2);
	} else {
		throw ExecutionError("Cannot divide " + getTypeDesc() + " by " + other.getTypeDesc());
	}
}

Value Value::multiply(Value other) {
	if(isInt() && other.isInt()) {
		return Value(getInt() * other.getInt());
	} else if(isNumeric() && other.isNumeric()) {
		return Value(convertToDouble() * other.convertToDouble());
	} else {
		throw ExecutionError("Cannot multiply " + getTypeDesc() + " by " + other.getTypeDesc());
	}
}

Value Value::modulo(Value other) {
	if(isInt() && other.isInt()) {
		return Value(getInt() % other.getInt());
	} else if(isNumeric() && other.isNumeric()) {
		return Value(fmod(convertToDouble(), other.convertToDouble()));
	} else {
		throw ExecutionError("Cannot multiply " + getTypeDesc() + " by " + other.getTypeDesc());
	}
}

int intPow(int32_t x, int32_t p) {
	if(p == 0) return 1;
	if(p == 1) return x;
	
	int32_t halfPow = intPow(x, p/2);
	if(p % 2 == 0) return halfPow * halfPow;
	else return x * halfPow * halfPow;
}

Value Value::power(Value other) {
	if(isInt() && other.isInt() && other.getInt() >= 0) {
		return Value(intPow(getInt(), other.getInt()));
	} else if(isNumeric() && other.isNumeric()) {
		return Value(std::pow(convertToDouble(), other.convertToDouble()));
	} else {
		throw ExecutionError("Cannot multiply " + getTypeDesc() + " by " + other.getTypeDesc());
	}
}

bool Value::equals(Value other) {
	if(isInt() && other.isInt()) return getInt() == other.getInt();
	else if(isNumeric()) return other.isNumeric() && convertToDouble() == other.convertToDouble();
	else if(isNil()) return other.isNil();
	else if(isBool()) return other.isBool() && getBool() == other.getBool();
	else if(isObject()) return other.isObject() && getObject()->equals(*other.getObject());
	else throw std::runtime_error("Invalid value");
}

bool Value::less(Value other) {
	if(isInt() && other.isInt()) return getInt() < other.getInt();
	else if(isNumeric() && other.isNumeric()) return convertToDouble() < other.convertToDouble();
	else throw ExecutionError("Cannot test if " + getTypeDesc() + " is less than " + other.getTypeDesc());
}

bool Value::less_or_eq(Value other) {
	if(isInt() && other.isInt()) return getInt() <= other.getInt();
	else if(isNumeric() && other.isNumeric()) return convertToDouble() <= other.convertToDouble();
	else throw ExecutionError("Cannot test if " + getTypeDesc() + " is less than or equal to " + other.getTypeDesc());
}

std::string Value::getTypeDesc() {
	if(isNil()) {
		return "nil";
	} else if(isBool()) {
		return "bool";
	} else if(isInt()) {
		return "int";
	} else if(isReal()) {
		return "real";
	} else if(isObject()) {
		return getObject()->getTypeDesc();
	} else {
		throw std::runtime_error("Invalid value");
	}
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
	} else if(isObject()) {
		return getObject()->toString();
	} else {
		throw std::runtime_error("Invalid value");
	}
}


Value Object::negate() {
	throw ExecutionError("Can't negate " + getTypeDesc());
}

Value Object::plus(Value other) {
	throw ExecutionError("Cannot add " + getTypeDesc() + " to " + other.getTypeDesc());
}

bool Object::equals(Object& obj) {
	return this == &obj;
}

std::string Object::toString() {
	std::stringstream ss;
	ss << "<" << getTypeDesc() << " " << this << ">";
	return ss.str();
}


void Namespace::markChildren() {
	for(auto& pair : map) {
		pair.second.mark();
	}
}


List::List(std::vector<Value>&& vec) : vec(std::move(vec)) {}

void List::markChildren() {
	for(Value val : vec) {
		val.mark();
	}
}

std::string List::toString() {
	std::string res = "[";
	for(uint32_t i = 0; i < vec.size(); i++) {
		res += vec[i].toString();
		if(i != vec.size()-1)
			res += ", ";
	}
	res += "]";
	return res;
}


String::String(std::string str) : str(str) {}

Value String::plus(Value other) {
	String* otherStr;
	if(otherStr = other.get<String>())
		throw ExecutionError("Cannot add string to " + other.getTypeDesc());
	return Value(new String(str + otherStr->str));
}

bool String::equals(Object& obj) {
	return str == static_cast<String&>(obj).str;
}

std::string String::toString() {
	return escapeString(str);
}


CFunction::CFunction(std::function<Value(std::vector<Value>&)> func) : func(func) {}


ExecutionRecord::ExecutionRecord(uint32_t localBase, uint32_t localCnt, Function* func)
	: localBase(localBase), localCnt(localCnt), funcIdx(0), codeOffset(0), func(func) {
	if(func) {
		GC::pin(func);
	}
}

ExecutionRecord::~ExecutionRecord() {
	if(func) {
		GC::unpin(func);
	}
}


Upvalue::Upvalue(Value* local, ExecutionRecord* record, uint16_t localIdx)
	: pointer(local), record(record), localIdx(localIdx) {}

Upvalue::~Upvalue() {
	if(record) {
		record->upvalueBackPointers.erase(localIdx);
	}
}

void Upvalue::markChildren() { storage.mark(); }

void Upvalue::close() {
	storage = *pointer;
	pointer = &storage;
	record = nullptr;
}


Function::Function(uint16_t protoIdx, uint16_t argCnt, uint16_t upvalueCnt)
	: protoIdx(protoIdx), argCnt(argCnt) {
	upvalues.resize(upvalueCnt);
}

void Function::markChildren() {
	for(Upvalue* upvalue : upvalues) {
		upvalue->mark();
	}
}
