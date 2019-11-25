#pragma once

#include "vm/value.hpp"

class Type {
public:
	virtual bool canBeAssignedTo(Type& other);
};

class UnknownType : public Type {
public:
	bool canBeAssignedTo(Type& other) override;
};

class Subtype : public Type {
public:
	Subtype(Type& parent);
	
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
