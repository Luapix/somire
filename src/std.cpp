#include "std.hpp"

#include <string>
#include <iostream>

void checkTypes(std::vector<Value>& values, std::vector<ValueType> types) {
	if(values.size() != types.size())
		throw ExecutionError("Expected " + std::to_string(types.size()) + " arguments, got " + std::to_string(values.size()));
	for(uint32_t i = 0; i < values.size(); i++) {
		ValueType type = values[i].type();
		if(type != types[i])
			throw ExecutionError("Expected a " + valueTypeDesc(types[i]) + " for argument " + std::to_string(i) + ", got " + valueTypeDesc(type));
	}
}

Value log(std::vector<Value>& args) {
	for(uint32_t i = 0; i < args.size(); i++) {
		std::cout << args[i].toString();
		if(i != args.size() - 1)
			std::cout << " ";
	}
	std::cout << std::endl;
	return Value();
}

Value writeLine(std::vector<Value>& args) {
	checkTypes(args, { ValueType::STR });
	std::cout << static_cast<String*>(args[0].getPointer())->str << std::endl;
	return Value();
}

void loadStd(Namespace& ns) {
	ns.map["log"] = Value(new CFunction(log));
	ns.map["writeLine"] = Value(new CFunction(writeLine));
}
