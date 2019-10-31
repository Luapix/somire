#pragma once

#include <ios>
#include <iostream>

template <typename O>
void Chunk::writeToFile(O& output) {
	output.write((const char*) magicBytes.data(), magicBytes.size());
	
	uint32_t constantCnt = constants.size();
	output.write((const char*) &constantCnt, sizeof(uint32_t));
	
	for(std::unique_ptr<Value>& cnst : constants) {
		writeConstantToFile(output, *cnst);
	}
	
	output.write((const char*) bytecode.data(), bytecode.size());
}

template <typename I>
std::unique_ptr<Chunk> Chunk::loadFromFile(I& input) {
	input.exceptions(std::ios_base::failbit);
	
	std::unique_ptr<Chunk> chunk(new Chunk());
	
	std::array<uint8_t, magicBytes.size()> buffer;
	input.read((char*) buffer.data(), buffer.size());
	if((int) input.gcount() != (int) buffer.size()) {
		throw ExecutionError("Unexpected EOF in bytecode file");
	}
	if(buffer != magicBytes) {
		throw ExecutionError("Invalid Somiré bytecode file");
	}
	
	uint32_t constantCnt;
	input.read((char*) &constantCnt, sizeof(uint32_t));
	
	std::cout << "Loading " << constantCnt << " constants..." << std::endl;
	
	for(uint32_t i = 0; i < constantCnt; i++) {
		chunk->loadConstantFromFile(input);
		std::cout << ":: " << chunk->constants.back()->toString() << std::endl;
	}
	
	size_t bytecodeStart = (size_t) input.tellg();
	
	input.seekg(0, std::ios::end);
	size_t size = (size_t) input.tellg() - bytecodeStart;
	chunk->bytecode.resize(size);
	
	input.seekg(bytecodeStart, std::ios::beg);
	input.read((char*) chunk->bytecode.data(), size);
	
	return chunk;
}

template <typename O>
void Chunk::writeConstantToFile(O& output, Value& val) {
	ValueType type = val.type;
	output.write((const char*) &type, sizeof(ValueType));
	
	switch(type) {
	case ValueType::NIL:
		break;
	case ValueType::INT:
		output.write((const char*) &static_cast<ValueInt&>(val).val, sizeof(int32_t));
		break;
	default:
		throw ExecutionError("Constant serialization is unimplemented for this type");
	}
}

template <typename I>
void Chunk::loadConstantFromFile(I& input) {
	ValueType type;
	input.read((char*) &type, sizeof(ValueType));
	
	switch(type) {
	case ValueType::NIL:
		constants.push_back(std::unique_ptr<Value>(new Value(type)));
		break;
	case ValueType::INT:
		int32_t val;
		input.read((char*) &val, sizeof(int32_t));
		constants.push_back(std::unique_ptr<Value>(new ValueInt(val)));
		break;
	default:
		throw ExecutionError("Constant unserialization is unimplemented for type " + std::to_string((int) type));
	}
}
