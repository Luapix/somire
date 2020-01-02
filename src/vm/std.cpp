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
	return Value(new String(args[0].toString()));
}

Value write(std::vector<Value>& args) {
	String& s = *args[0].get<String>();
	std::cout << s.str;
	return Value::nil();
}

Value writeLine(std::vector<Value>& args) {
	String& s = *args[0].get<String>();
	std::cout << s.str << std::endl;
	return Value::nil();
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
	List& list = *args[0].get<List>();
	return Value((int32_t) list.vec.size());
}

Value toBool(std::vector<Value>& args) {
	if(!args[0].isBool())
		throw ExecutionError("Cannot convert " + args[0].toString() + " to bool");
	return args[0];
}

void loadStd(Namespace& ns) {
	ns.map["log"] = Value(new CFunction(log));
	ns.map["repr"] = Value(new CFunction(repr));
	ns.map["write"] = Value(new CFunction(write));
	ns.map["writeLine"] = Value(new CFunction(writeLine));
	ns.map["bool"] = Value(new CFunction(toBool));
	
	Namespace* listNs = new Namespace();
	ns.map["list"] = listNs;
	listNs->map["add"] = Value(new CFunction(listAdd));
	listNs->map["size"] = Value(new CFunction(listSize));
}

void defineStdTypes(TypeNamespace& ns, TypeNamespace& types) {
	ns.map["log"] = types.map["macro"];
	ns.map["repr"] = new FunctionType({types.map["any"]}, types.map["string"]);
	ns.map["write"] = new FunctionType({types.map["string"]}, types.map["nil"]);
	ns.map["writeLine"] = new FunctionType({types.map["string"]}, types.map["nil"]);
	ns.map["bool"] = new FunctionType({types.map["any"]}, types.map["bool"]);
}
