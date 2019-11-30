#pragma once

#include "vm/value.hpp"

class Type : public Object {
public:
	Type(std::string name);
	
	virtual bool canBeAssignedTo(Type* other) = 0;
	virtual std::string getDesc();
	
	std::string getTypeDesc() override { return "type"; }
	std::string toString() override { return "<type " + getDesc() + ">"; }
	
private:
	std::string name;
};

class AnyType : public Type {
public:
	AnyType();
	
	bool canBeAssignedTo(Type* other) override;
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

class TypeNamespace : public Object {
public:
	std::unordered_map<std::string, Type*> map;
	
	void markChildren() override;
};




void defineBasicTypes(TypeNamespace& ns);
