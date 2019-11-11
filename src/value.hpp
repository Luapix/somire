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
	REAL,
	LIST,
	STR
};

std::string valueTypeDesc(ValueType type);


class Object;

class Value {
public:
	Value() = default;
	Value(int32_t integer);
	Value(double real);
	Value(Object* obj);
	
	static Value nil();
	
	void mark();
	
	ValueType type();
	
	bool isNil();
	bool isInt();
	bool isDouble();
	bool isPointer();
	
	int32_t getInt();
	double getReal();
	Object* getPointer();
	
	Value negate();
	Value plus(Value other);
	
	std::string toString();
	
private:
	union {
		double asDouble;
		uint64_t asBits;
	};
	
	static const uint64_t POINTER_TAG = 0xfff8000000000000;
	static const uint64_t INT_TAG     = 0xfff9000000000000;
	static const uint64_t NIL         = 0xfffa000000000000;
};

class Object : public GC::GCObject {
public:
	const ValueType type;
	
	Object(ValueType type);
	virtual ~Object() = default;
	
	virtual Value negate();
	virtual Value plus(Value other);
	
	virtual std::string toString();
};

class List : public Object {
public:
	std::vector<Value> vec;
	
	List();
	
	void markChildren() override;
};

class String : public Object {
public:
	std::string str;
	
	String(std::string str);
};
