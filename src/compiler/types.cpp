#include "types.hpp"

Type::Type(std::string desc) : desc(desc) {}

std::string Type::getDesc() { return desc; }


AnyType::AnyType() : Type("any") {}

bool AnyType::canBeAssignedTo(Type& other) {
	return this == &other;
}


UnknownType::UnknownType() : Type("unknown") {}

bool UnknownType::canBeAssignedTo(Type& other) {
	return false;
};


Subtype::Subtype(std::string desc, Type& parent) : Type(desc), parent(parent) {}

bool Subtype::canBeAssignedTo(Type& other) {
	return this == &other || parent.canBeAssignedTo(other);
}


AnyType anyType;

Subtype nilType("nil", anyType);
Subtype boolType("bool", anyType);
Subtype realType("real", anyType);
Subtype intType("int", realType);

UnknownType unknownObjectType;
Subtype listType("list", anyType);
Subtype stringType("string", anyType);
Subtype functionType("function", anyType);
