#include "chunk.hpp"

#include <stdexcept>
#include <sstream>
#include <limits>
#include <fstream>

CompileError::CompileError(const std::string& what)
	: runtime_error("Compile error: " + what) { }

std::unordered_map<Opcode, std::string> opcodeDescTable = {
	{Opcode::IGNORE, "IGNORE"},
	{Opcode::CONSTANT, "CONSTANT"},
	{Opcode::UNI_MINUS, "UNI_MINUS"},
	{Opcode::BIN_PLUS, "BIN_PLUS"},
	{Opcode::LET, "LET"},
	{Opcode::POP, "POP"},
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

Chunk::Chunk() : constants(new List()), codeOut(bytecode) {}

void Chunk::fillInJump(uint32_t pos) {
	int32_t relJmp = bytecode.size() - (int32_t) (pos + 2);
	if(relJmp < std::numeric_limits<int16_t>::min() || std::numeric_limits<int16_t>::max() < relJmp)
		throw std::runtime_error("Relative jump outside of int16_t limits");
	int16_t x = (int16_t) relJmp;
	auto it = &bytecode[pos];
	writeI16(it, x);
}

void Chunk::writeToFile(std::ofstream& fs) {
	fs.write((const char*) magicBytes.data(), magicBytes.size());
	
	std::ostream_iterator<char> it(fs);
	
	if(constants->vec.size() > 0xffff)
		throw std::runtime_error("Too many constants in chunk");
	uint16_t constantCnt = (uint16_t) constants->vec.size();
	writeUI16(it, constantCnt);
	
	for(Value cnst : constants->vec) {
		writeConstantToFile(it, cnst);
	}
	
	fs.write((const char*) bytecode.data(), bytecode.size());
}

std::unique_ptr<Chunk> Chunk::loadFromFile(std::ifstream& fs) {
	if(!fs.is_open()) throw ExecutionError("Unable to open file");
	fs.exceptions(std::ios_base::failbit);
	
	std::array<uint8_t, magicBytes.size()> buffer;
	fs.read((char*) buffer.data(), buffer.size());
	if((int) fs.gcount() != (int) buffer.size()) {
		throw std::runtime_error("Unexpected EOF in bytecode file");
	}
	if(buffer != magicBytes) {
		throw std::runtime_error("Invalid Somiré bytecode file");
	}
	
	std::unique_ptr<Chunk> chunk(new Chunk());
	std::istreambuf_iterator<char> it(fs);
	
	uint16_t constantCnt = readUI16(it);
	for(uint8_t i = 0; i < constantCnt; i++) {
		chunk->loadConstantFromFile(it);
	}
	
	size_t bytecodeStart = (size_t) fs.tellg();
	fs.seekg(0, std::ios::end);
	size_t size = (size_t) fs.tellg() - bytecodeStart;
	chunk->bytecode.resize(size);
	
	fs.seekg(bytecodeStart, std::ios::beg);
	fs.read((char*) chunk->bytecode.data(), size);
	
	return chunk;
}

std::string Chunk::list() {
	std::stringstream res;
	
	res << "Constants:\n";
	for(uint32_t i = 0; i < constants->vec.size(); i++) {
		res << i << ": " << constants->vec[i].toString() << "\n";
	}
	
	res << "\nCode:\n";
	auto it = bytecode.begin();
	while(it != bytecode.end()) {
		Opcode op = static_cast<Opcode>(readUI8(it));
		res << opcodeDesc(op);
		switch(op) {
		case Opcode::CONSTANT:
		case Opcode::SET:
		case Opcode::LOCAL:
		case Opcode::POP:
			res << " " << (int) readUI16(it);
			break;
		case Opcode::JUMP_IF_NOT:
			res << " " << (int) readI16(it);
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
			break;
		}
		res << "\n";
	}
	
	return res.str();
}
