#pragma once

#include "vm/value.hpp"

class Type {
public:
	Type(std::string desc);
	
	virtual bool canBeAssignedTo(Type& other) = 0;
	virtual std::string getDesc();
	
private:
	std::string desc;
};

class AnyType : public Type {
public:
	AnyType();
	
	bool canBeAssignedTo(Type& other) override;
};

class UnknownType : public Type {
public:
	UnknownType();
	
	bool canBeAssignedTo(Type& other) override;
};

class Subtype : public Type {
public:
	Subtype(std::string desc, Type& parent);
	
	bool canBeAssignedTo(Type& other) override;
	
private:
	Type& parent;
};

extern AnyType anyType;
extern Subtype nilType;
extern Subtype boolType;
extern Subtype realType;
extern Subtype intType;

extern UnknownType unknownObjectType;
extern Subtype listType;
extern Subtype stringType;
extern Subtype functionType;
