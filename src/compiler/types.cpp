#include "types.hpp"

bool Type::canBeAssignedTo(Type& other) {
	return this == &other;
}

bool UnknownType::canBeAssignedTo(Type& other) { return false; };

Subtype::Subtype(Type& parent) : parent(parent) {}

bool Subtype::canBeAssignedTo(Type& other) {
	return this == &other || parent.canBeAssignedTo(other);
}

Type nilType;
Type boolType;
Type realType;
Subtype intType(realType);

UnknownType unknownObjectType;
Type listType;
Type stringType;
Type functionType;
