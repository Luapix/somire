#pragma once

#include "vm/value.hpp"

class Type : public Object {
public:
	Type(std::string name, bool isAny = false);
	
	bool isAny() { return _isAny; }
	
	virtual bool canBeAssignedTo(Type* other);
	virtual std::string getDesc();
	
	std::string getTypeDesc() override { return "type"; }
	std::string toString() override { return "<type " + getDesc() + ">"; }
	
private:
	std::string name;
	bool _isAny;
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
};


class TypeNamespace : public Object {
public:
	std::unordered_map<std::string, Type*> map;
	
	void markChildren() override;
};


void defineBasicTypes(TypeNamespace& ns);
