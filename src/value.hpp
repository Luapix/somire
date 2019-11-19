#pragma once

#include <stdexcept>
#include <cstdint>
#include <functional>

#include "gc.hpp"

class ExecutionError : public std::runtime_error {
public:
	ExecutionError(const std::string& what);
};

enum class ValueType : uint8_t {
	NIL,
	BOOL,
	INT,
	REAL,
	STR,
	INTERNAL,
	C_FUNC,
	FUNC
};

std::string valueTypeDesc(ValueType type);


class Object;

class Value {
public:
	Value();
	Value(bool boolean);
	Value(int32_t integer);
	Value(double real);
	Value(Object* obj);
	
	static Value nil();
	
	void mark();
	
	ValueType type();
	
	inline bool isNil() { return asBits == NIL; }
	inline bool isBool() { return (asBits & TAG_MASK) == BOOL_TAG; }
	inline bool isInt() { return (asBits & TAG_MASK) == INT_TAG; }
	inline bool isReal() { return asBits <= POINTER_TAG; }
	inline bool isPointer() { return (asBits & TAG_MASK) == POINTER_TAG && asBits != POINTER_TAG; }

	inline bool isNumeric() { return isInt() || isReal(); }
	inline double convertToDouble() { return isInt() ? (double) getInt() : asDouble; }

	inline bool getBool() { return (bool) (asBits & 0x1); }
	inline int32_t getInt() { return (int32_t) ((uint32_t) asBits); }
	inline double getReal() { return asDouble; }
	inline Object* getPointer() { return reinterpret_cast<Object*>(asBits & 0xffffffffffff); }
	
	Value negate();
	Value plus(Value other);
	Value minus(Value other);
	Value divide(Value other);
	Value multiply(Value other);
	
	bool equals(Value other);
	bool less(Value other);
	bool less_or_eq(Value other);
	
	std::string toString();
	
private:
	union {
		double asDouble;
		uint64_t asBits;
	};
	
	static const uint64_t TAG_MASK    = 0xffff000000000000;
	static const uint64_t POINTER_TAG = 0xfff8000000000000; // Careful, the 0 pointer is actually NaN
	static const uint64_t INT_TAG     = 0xfff9000000000000;
	static const uint64_t NIL         = 0xfffa000000000000;
	static const uint64_t BOOL_TAG    = 0xfffb000000000000;
};

class Object : public GC::GCObject {
public:
	const ValueType type;
	
	Object(ValueType type);
	virtual ~Object() = default;
	
	virtual Value negate();
	virtual Value plus(Value other);
	
	virtual bool equals(Object& obj);
	
	virtual std::string toString();
};

class Namespace : public Object {
public:
	std::unordered_map<std::string, Value> map;
	
	Namespace();
	
	void markChildren() override;
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
	
	bool equals(Object& obj) override;
	
	std::string toString() override;
};

class CFunction : public Object {
public:
	std::function<Value(std::vector<Value>&)> func;
	
	CFunction(std::function<Value(std::vector<Value>&)> func);
};


class Upvalue;
class Function;

struct ExecutionRecord {
	uint32_t localBase;
	uint32_t localCnt;
	
	uint32_t funcIdx;
	uint32_t codeOffset;
	
	Function* func;
	std::unordered_map<uint16_t, Upvalue*> upvalueBackPointers;
	
	ExecutionRecord(uint32_t localBase, uint32_t localCnt, Function* func = nullptr);
	~ExecutionRecord();
};

class Upvalue : public GC::GCObject {
public:
	Upvalue(Value* local, ExecutionRecord* record, uint16_t localIdx);
	~Upvalue();
	
	void markChildren() override;
	
	inline Value& resolve() { return *pointer; }
	void close();
	
private:
	Value* pointer;
	Value storage;
	
	ExecutionRecord* record;
	uint16_t localIdx;
};

class Function : public Object {
public:
	uint16_t protoIdx;
	uint16_t argCnt;
	std::vector<Upvalue*> upvalues;
	
	Function(uint16_t protoIdx, uint16_t argCnt, uint16_t upvalueCnt);
	
	void markChildren() override;
};
