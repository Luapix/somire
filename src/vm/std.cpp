#include "std.hpp"

#include <string>
#include <iostream>

void checkNumber(std::vector<Value>& values, uint32_t number) {
	if(values.size() != number)
		throw ExecutionError("Expected " + std::to_string(number) + " arguments, got " + std::to_string(values.size()));
}

void checkTypes(std::vector<Value>& values, std::vector<ValueType> types) {
	checkNumber(values, types.size());
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

Value repr(std::vector<Value>& args) {
	checkNumber(args, 1);
	return Value(new String(args[0].toString()));
}

Value write(std::vector<Value>& args) {
	checkTypes(args, { ValueType::STR });
	std::cout << static_cast<String*>(args[0].getPointer())->str;
	return Value();
}

Value writeLine(std::vector<Value>& args) {
	checkTypes(args, { ValueType::STR });
	std::cout << static_cast<String*>(args[0].getPointer())->str << std::endl;
	return Value();
}

Value listNew(std::vector<Value>& args) {
	checkNumber(args, 0);
	return Value(new List());
}

Value listAdd(std::vector<Value>& args) {
	if(args.size() <= 1 || args.size() >= 4)
		throw ExecutionError("Expected 2 or 3 arguments, got " + std::to_string(args.size()));
	if(args[0].type() != ValueType::LIST)
		throw ExecutionError("Expected a list for argument 0, got " + valueTypeDesc(args[0].type()));
	List& list = static_cast<List&>(*args[0].getPointer());
	if(args.size() == 2) {
		list.vec.push_back(args[1]);
	} else {
		if(args[2].type() != ValueType::INT)
			throw ExecutionError("Expected a int for argument 2, got " + valueTypeDesc(args[2].type()));
		int32_t pos = args[2].getInt();
		if(pos < 1)
			throw ExecutionError("Provided list index is negative");
		if(pos > list.vec.size() + 1)
			throw ExecutionError("Provided list index is past the end");
		list.vec.insert(list.vec.begin() + (pos - 1), args[1]);
	}
	return Value();
}

Value listSize(std::vector<Value>& args) {
	checkTypes(args, { ValueType::LIST });
	return Value((int32_t) static_cast<List&>(*args[0].getPointer()).vec.size());
}

void loadStd(Namespace& ns) {
	ns.map["log"] = Value(new CFunction(log));
	ns.map["repr"] = Value(new CFunction(repr));
	ns.map["write"] = Value(new CFunction(write));
	ns.map["writeLine"] = Value(new CFunction(writeLine));
	ns.map["listNew"] = Value(new CFunction(listNew));
	ns.map["listAdd"] = Value(new CFunction(listAdd));
	ns.map["listSize"] = Value(new CFunction(listSize));
}
