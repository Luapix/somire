#pragma once

#include "vm/value.hpp"

class Type;

class TypeNamespace : public Object {
public:
	std::unordered_map<std::string, Type*> map;
	
	void markChildren() override;
};

class Type : public Object {
public:
	Type(std::string name, bool isAny = false);
	
	bool isAny() { return _isAny; }
	
	void defineMethod(std::string methodName, Type* methodType);
	Type* getMethod(std::string methodName);
	
	virtual bool canBeAssignedTo(Type* other);
	virtual std::string getDesc();
	
	std::string getTypeDesc() override { return "type"; }
	std::string toString() override { return "<type " + getDesc() + ">"; }
	
	void markChildren() override;
	
private:
	std::string name;
	bool _isAny;
	std::unordered_map<std::string, Type*> methods;
};

class UnknownType : public Type {
public:
	UnknownType();
	
	bool canBeAssignedTo(Type* other) override;
};

class Subtype : public Type {
public:
	Subtype(std::string name, Type* parent);
	
	bool canBeAssignedTo(Type* other) override;
	
	void markChildren() override;
	
private:
	Type* parent;
};

class FunctionType : public Type {
public:
	std::vector<Type*> argTypes;
	Type* resType;
	
	FunctionType(std::vector<Type*> argTypes, Type* resType);
	
	bool canBeAssignedTo(Type* other) override;
	std::string getDesc() override;
	
	void markChildren() override;
};

class ListType : public Type {
public:
	Type* elemType; // nullptr represents empty list
	
	ListType(Type* elemType);
	
	bool canBeAssignedTo(Type* other) override;
	std::string getDesc() override;
	
	void markChildren() override;
};


void defineBasicTypes(TypeNamespace& ns);
