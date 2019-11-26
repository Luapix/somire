#pragma once

#include "vm/value.hpp"

class Type {
public:
	Type(std::string desc);
	
	virtual bool canBeAssignedTo(Type& other);
	virtual std::string getDesc();
	
private:
	std::string desc;
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

extern Type nilType;
extern Type boolType;
extern Type realType;
extern Subtype intType;

extern UnknownType unknownObjectType;
extern Type listType;
extern Type stringType;
extern Type functionType;
