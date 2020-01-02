#pragma once

#include <stdexcept>
#include <cstdint>
#include <functional>

#include "util/gc.hpp"

class ExecutionError : public std::runtime_error {
public:
	ExecutionError(const std::string& what);
};


class Object;

class Value {
public:
	Value() = default;
	static Value nil();
	Value(bool boolean);
	Value(int32_t integer);
	Value(double real);
	Value(Object* obj);
	
	inline bool isNil() { return asBits == NIL; }
	inline bool isBool() { return (asBits & TAG_MASK) == BOOL_TAG; }
	inline bool isInt() { return (asBits & TAG_MASK) == INT_TAG; }
	inline bool isReal() { return asBits <= OBJECT_TAG; }
	inline bool isObject() { return (asBits & TAG_MASK) == OBJECT_TAG && asBits != OBJECT_TAG; }
	
	inline bool isNumeric() { return isInt() || isReal(); }
	inline double convertToDouble() { return isInt() ? (double) getInt() : asDouble; }

	inline bool getBool() { return (bool) (asBits & 0x1); }
	inline int32_t getInt() { return (int32_t) ((uint32_t) asBits); }
	inline double getReal() { return asDouble; }
	inline Object* getObject() { return reinterpret_cast<Object*>(asBits & 0xffffffffffff); }
	
	template<typename T>
	inline T* get() { return isObject() ? dynamic_cast<T*>(getObject()) : nullptr; }
	
	
	void mark();
	
	Value negate();
	Value plus(Value other);
	Value minus(Value other);
	Value divide(Value other);
	Value multiply(Value other);
	Value modulo(Value other);
	Value power(Value other);
	
	bool equals(Value other);
	bool less(Value other);
	bool less_or_eq(Value other);
	
	std::string getTypeDesc();
	std::string toString();
	
private:
	static const uint64_t TAG_MASK    = 0xffff000000000000;
	static const uint64_t OBJECT_TAG = 0xfff8000000000000; // Careful, the 0 pointer is actually NaN
	static const uint64_t INT_TAG     = 0xfff9000000000000;
	static const uint64_t NIL         = 0xfffa000000000000;
	static const uint64_t BOOL_TAG    = 0xfffb000000000000;
	
	union {
		double asDouble;
		uint64_t asBits;
	};
	
	inline static Value fromBits(int64_t bits) {
		Value val;
		val.asBits = bits;
		return val;
	}
};

class Object : public GC::GCObject {
public:
	virtual ~Object() = default;
	
	virtual Value negate();
	virtual Value plus(Value other);
	
	virtual bool equals(Object& obj);
	
	virtual std::string getTypeDesc() { return "unknown object"; }
	virtual std::string toString();
};

class Namespace : public Object {
public:
	std::unordered_map<std::string, Value> map;
	
	void markChildren() override;
};

class List : public Object {
public:
	std::vector<Value> vec;
	
	List() = default;
	List(std::vector<Value>&& vec);
	
	std::string getTypeDesc() override { return "list"; }
	std::string toString() override;
	
	void markChildren() override;
};

class String : public Object {
public:
	std::string str;
	
	String(std::string str);
	
	Value plus(Value other) override;
	
	bool equals(Object& obj) override;
	
	std::string getTypeDesc() override { return "string"; }
	std::string toString() override;
};

class CFunction : public Object {
public:
	std::function<Value(std::vector<Value>&)> func;
	
	CFunction(std::function<Value(std::vector<Value>&)> func);
	
	std::string getTypeDesc() override { return "C function"; }
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
	
	std::string getTypeDesc() override { return "function"; }
	
	void markChildren() override;
};

class Method : public Object {
public:
	Value self;
	CFunction* function;
	
	Method(Value self, CFunction* function);
	
	std::string getTypeDesc() override { return "method"; }
	
	void markChildren() override;
};
