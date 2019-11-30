#include "std.hpp"

#include <string>
#include <iostream>

void checkNumber(std::vector<Value>& values, uint32_t number) {
	if(values.size() != number)
		throw ExecutionError("Expected " + std::to_string(number) + " arguments, got " + std::to_string(values.size()));
}

void expectType(Value val, bool isType, int argument, std::string expectedType) {
	if(!isType)
		throw ExecutionError("Expected a " + expectedType + " for argument " + std::to_string(argument) + ", got " + val.getTypeDesc());
}

template<typename T>
T& expectObject(Value val, int argument, std::string expectedType) {
	T* obj = val.get<T>();
	if(!obj)
		throw ExecutionError("Expected a " + expectedType + " for argument " + std::to_string(argument) + ", got " + val.getTypeDesc());
	return *obj;
}

Value log(std::vector<Value>& args) {
	for(uint32_t i = 0; i < args.size(); i++) {
		std::cout << args[i].toString();
		if(i != args.size() - 1)
			std::cout << " ";
	}
	std::cout << std::endl;
	return Value::nil();
}

Value repr(std::vector<Value>& args) {
	checkNumber(args, 1);
	return Value(new String(args[0].toString()));
}

Value write(std::vector<Value>& args) {
	checkNumber(args, 1);
	String& s = expectObject<String>(args[0], 0, "string");
	std::cout << s.str;
	return Value::nil();
}

Value writeLine(std::vector<Value>& args) {
	checkNumber(args, 1);
	String& s = expectObject<String>(args[0], 0, "string");
	std::cout << s.str << std::endl;
	return Value::nil();
}

Value listNew(std::vector<Value>& args) {
	checkNumber(args, 0);
	return Value(new List());
}

Value listAdd(std::vector<Value>& args) {
	if(args.size() <= 1 || args.size() >= 4)
		throw ExecutionError("Expected 2 or 3 arguments, got " + std::to_string(args.size()));
	List& list = expectObject<List>(args[0], 0, "list");
	if(args.size() == 2) {
		list.vec.push_back(args[1]);
	} else {
		expectType(args[2], args[2].isInt(), 2, "int");
		int32_t pos = args[2].getInt();
		if(pos < 1)
			throw ExecutionError("Provided list index is negative");
		if(pos > list.vec.size() + 1)
			throw ExecutionError("Provided list index is past the end");
		list.vec.insert(list.vec.begin() + (pos - 1), args[1]);
	}
	return Value::nil();
}

Value listSize(std::vector<Value>& args) {
	checkNumber(args, 1);
	List& list = expectObject<List>(args[0], 0, "list");
	return Value((int32_t) list.vec.size());
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

void defineStdTypes(TypeNamespace& ns, TypeNamespace types) {
	Type* functionType = types.map["function"];
	ns.map["log"] = functionType;
	ns.map["repr"] = functionType;
	ns.map["write"] = functionType;
	ns.map["writeLine"] = functionType;
	ns.map["listNew"] = functionType;
	ns.map["listAdd"] = functionType;
	ns.map["listSize"] = functionType;
}
