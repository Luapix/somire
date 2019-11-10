#pragma once

#include <ios>
#include <iostream>

template <typename O>
void Chunk::writeToFile(O& output) {
	output.write((const char*) magicBytes.data(), magicBytes.size());
	
	if(constants->vec.size() > 0xff)
		throw std::runtime_error("Too many constants in chunk");
	uint8_t constantCnt = constants->vec.size();
	output.write((const char*) &constantCnt, sizeof(constantCnt));
	
	for(Value* cnst : constants->vec) {
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
		throw std::runtime_error("Unexpected EOF in bytecode file");
	}
	if(buffer != magicBytes) {
		throw std::runtime_error("Invalid Somiré bytecode file");
	}
	
	uint8_t constantCnt;
	input.read((char*) &constantCnt, sizeof(constantCnt));
	
	for(uint8_t i = 0; i < constantCnt; i++) {
		chunk->loadConstantFromFile(input);
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
	case ValueType::INT: {
		std::array<uint8_t, 4> buf = serializeUInt((uint32_t) static_cast<ValueInt&>(val).val);
		output.write((const char*) buf.data(), buf.size());
		break;
	} case ValueType::REAL: {
		std::array<uint8_t, 8> buf = serializeReal(static_cast<ValueReal&>(val).val);
		output.write((const char*) buf.data(), buf.size());
		break;
	} default:
		throw std::runtime_error("Constant serialization is unimplemented for this type");
	}
}

template <typename I>
void Chunk::loadConstantFromFile(I& input) {
	ValueType type;
	input.read((char*) &type, sizeof(ValueType));
	
	switch(type) {
	case ValueType::NIL:
		constants->vec.push_back(new Value(type));
		break;
	case ValueType::INT: {
		std::array<uint8_t, 4> buf;
		input.read((char*) buf.data(), buf.size());
		int32_t val = (int32_t) parseUInt(buf);
		constants->vec.push_back(new ValueInt(val));
		break;
	} case ValueType::REAL: {
		std::array<uint8_t, 8> buf;
		input.read((char*) buf.data(), buf.size());
		double val = parseReal(buf);
		constants->vec.push_back(new ValueReal(val));
		break;
	} default:
		throw std::runtime_error("Constant unserialization is unimplemented for type " + std::to_string((int) type));
	}
}
