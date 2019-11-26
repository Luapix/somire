#include "types.hpp"

Type::Type(std::string desc) : desc(desc) {}

bool Type::canBeAssignedTo(Type& other) {
	return this == &other;
}

std::string Type::getDesc() { return desc; }


UnknownType::UnknownType() : Type("unknown") {}

bool UnknownType::canBeAssignedTo(Type& other) { return false; };


Subtype::Subtype(std::string desc, Type& parent) : Type(desc), parent(parent) {}

bool Subtype::canBeAssignedTo(Type& other) {
	return this == &other || parent.canBeAssignedTo(other);
}


Type nilType("nil");
Type boolType("bool");
Type realType("real");
Subtype intType("int", realType);

UnknownType unknownObjectType;
Type listType("list");
Type stringType("string");
Type functionType("function");
