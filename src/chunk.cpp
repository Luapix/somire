#include "chunk.hpp"

#include <stdexcept>
#include <sstream>

std::unordered_map<Opcode, std::string> opcodeDescTable = {
	{Opcode::IGNORE, "IGNORE"},
	{Opcode::CONSTANT, "CONSTANT"},
	{Opcode::UNI_MINUS, "UNI_MINUS"},
	{Opcode::BIN_PLUS, "BIN_PLUS"},
	{Opcode::LET, "LET"},
	{Opcode::SET, "SET"},
	{Opcode::LOCAL, "LOCAL"},
	{Opcode::LOG, "LOG"},
	{Opcode::NOT, "NOT"},
	{Opcode::AND, "AND"},
	{Opcode::OR, "OR"},
	{Opcode::EQUALS, "EQUALS"},
	{Opcode::JUMP_IF_NOT, "JUMP_IF_NOT"}
};

std::string opcodeDesc(Opcode opcode) {
	auto it = opcodeDescTable.find(opcode);
	if(it == opcodeDescTable.end())
		throw std::runtime_error("Unknown opcode");
	return it->second;
}

std::array<uint8_t, 4> serializeUInt(uint32_t x) {
	std::array<uint8_t, 4> res;
	for(uint8_t i = 0; i < res.size(); i++) {
		res[i] = x >> (i*8);
	}
	return res;
}

uint32_t parseUInt(std::array<uint8_t, 4> b) {
	uint32_t x = 0;
	for(uint8_t i = 0; i < b.size(); i++) {
		x |= ((uint32_t) b[i]) << (i*8);
	}
	return x;
}

std::array<uint8_t, 8> serializeReal(double x) {
	uint64_t& x2 = reinterpret_cast<uint64_t&>(x);
	std::array<uint8_t, 8> res;
	for(uint8_t i = 0; i < res.size(); i++) {
		res[i] = x2 >> (i*8);
	}
	return res;
}

double parseReal(std::array<uint8_t, 8> b) {
	uint64_t x2 = 0;
	for(uint8_t i = 0; i < b.size(); i++) {
		x2 |= ((uint64_t) b[i]) << (i*8);
	}
	return reinterpret_cast<double&>(x2);
}

Chunk::Chunk() : constants(new List()) {}

std::string Chunk::list() {
	std::stringstream res;
	
	res << "Constants:\n";
	for(uint32_t i = 0; i < constants->vec.size(); i++) {
		res << i << ": " << constants->vec[i].toString() << "\n";
	}
	
	res << "\nCode:\n";
	uint32_t i = 0;
	while(i < bytecode.size()) {
		Opcode op = static_cast<Opcode>(bytecode[i]);
		res << opcodeDesc(op);
		switch(op) {
		case Opcode::CONSTANT:
		case Opcode::SET:
		case Opcode::LOCAL:
		case Opcode::JUMP_IF_NOT:
			res << " " << (int) bytecode[i+1];
			i += 2;
			break;
		case Opcode::IGNORE:
		case Opcode::UNI_MINUS:
		case Opcode::BIN_PLUS:
		case Opcode::NOT:
		case Opcode::AND:
		case Opcode::OR:
		case Opcode::EQUALS:
		case Opcode::LET:
		case Opcode::LOG:
		default:
			i++;
			break;
		}
		res << "\n";
	}
	
	return res.str();
}
