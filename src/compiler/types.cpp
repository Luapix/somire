#include "types.hpp"

Type::Type(std::string name) : name(name) {}

std::string Type::getDesc() { return name; }


AnyType::AnyType() : Type("any") {}

bool AnyType::canBeAssignedTo(Type* other) {
	return this == other;
}


UnknownType::UnknownType() : Type("unknown") {}

bool UnknownType::canBeAssignedTo(Type* other) {
	return false;
};


Subtype::Subtype(std::string name, Type* parent) : Type(name), parent(parent) {}

bool Subtype::canBeAssignedTo(Type* other) {
	return this == other || parent->canBeAssignedTo(other);
}


void TypeNamespace::markChildren() {
	for(auto& pair : map) {
		pair.second->mark();
	}
}


void defineBasicTypes(TypeNamespace& ns) {
	Type* anyType = new AnyType();
	ns.map["any"] = anyType;
	
	ns.map["nil"] = new Subtype("nil", anyType);
	ns.map["bool"] = new Subtype("bool", anyType);
	ns.map["real"] = new Subtype("real", anyType);
	ns.map["int"] = new Subtype("int", ns.map["real"]);
	
	ns.map["unknown"] = new UnknownType();
	ns.map["list"] = new Subtype("list", anyType);
	ns.map["string"] = new Subtype("string", anyType);
	ns.map["function"] = new Subtype("function", anyType);
	ns.map["type"] = new Subtype("type", anyType);
}