#include "types.hpp"

Type::Type(std::string name, bool isAny) : name(name), _isAny(isAny) {}

bool Type::canBeAssignedTo(Type* other) {
	return this == other || other->isAny();
}

std::string Type::getDesc() { return name; }


UnknownType::UnknownType() : Type("unknown") {}

bool UnknownType::canBeAssignedTo(Type* other) {
	return false;
};


Subtype::Subtype(std::string name, Type* parent) : Type(name), parent(parent) {}

bool Subtype::canBeAssignedTo(Type* other) {
	return this == other || other->isAny() || parent->canBeAssignedTo(other);
}


FunctionType::FunctionType(std::vector<Type*> argTypes, Type* resType)
	: Type("function"), argTypes(argTypes), resType(resType) {}

bool FunctionType::canBeAssignedTo(Type* other) {
	if(other->isAny()) return true;
	FunctionType* other2 = dynamic_cast<FunctionType*>(other);
	if(!other2) return false;
	if(argTypes.size() != other2->argTypes.size()) return false;
	for(uint32_t i = 0; i < argTypes.size(); i++) {
		if(!other2->argTypes[i]->canBeAssignedTo(argTypes[i])) return false;
	}
	return resType->canBeAssignedTo(other2->resType);
}

std::string FunctionType::getDesc() {
	std::string res = "function(";
	for(uint32_t i = 0; i < argTypes.size(); i++) {
		res += argTypes[i]->getDesc();
		if(i != argTypes.size() - 1)
			res += ", ";
	}
	return res + ") -> " + resType->getDesc();
}


void TypeNamespace::markChildren() {
	for(auto& pair : map) {
		pair.second->mark();
	}
}


void defineBasicTypes(TypeNamespace& ns) {
	ns.map["any"] = new Type("any", true);
	
	ns.map["nil"] = new Type("nil");
	ns.map["bool"] = new Type("bool");
	ns.map["real"] = new Type("real");
	ns.map["int"] = new Subtype("int", ns.map["real"]);
	
	ns.map["unknown"] = new UnknownType();
	ns.map["list"] = new Type("list");
	ns.map["string"] = new Type("string");
	// For functions too complex to be described by a FunctionType
	ns.map["macro"] = new Type("macro");
	ns.map["type"] = new Type("type");
}