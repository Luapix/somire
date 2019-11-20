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
	{Opcode::BIN_MINUS, "BIN_MINUS"},
	{Opcode::MULTIPLY, "MULTIPLY"},
	{Opcode::DIVIDE, "DIVIDE"},
	{Opcode::MODULO, "MODULO"},
	{Opcode::LET, "LET"},
	{Opcode::POP, "POP"},
	{Opcode::SET_LOCAL, "SET_LOCAL"},
	{Opcode::LOCAL, "LOCAL"},
	{Opcode::GLOBAL, "GLOBAL"},
	{Opcode::NOT, "NOT"},
	{Opcode::AND, "AND"},
	{Opcode::OR, "OR"},
	{Opcode::EQUALS, "EQUALS"},
	{Opcode::LESS, "LESS"},
	{Opcode::LESS_OR_EQ, "LESS_OR_EQ"},
	{Opcode::JUMP_IF_NOT, "JUMP_IF_NOT"},
	{Opcode::JUMP, "JUMP"},
	{Opcode::CALL, "CALL"},
	{Opcode::RETURN, "RETURN"},
	{Opcode::MAKE_FUNC, "MAKE_FUNC"}
};

std::string opcodeDesc(Opcode opcode) {
	auto it = opcodeDescTable.find(opcode);
	if(it == opcodeDescTable.end())
		throw std::runtime_error("Unknown opcode");
	return it->second;
}

int16_t computeJump(uint32_t from, uint32_t to) {
	int32_t relJmp = (int32_t) to - (int32_t) from;
	if(relJmp < std::numeric_limits<int16_t>::min() || std::numeric_limits<int16_t>::max() < relJmp)
		throw std::runtime_error("Relative jump outside of int16_t limits");
	return (int16_t) relJmp;
}

FunctionChunk::FunctionChunk() : codeOut(code) {}

void FunctionChunk::fillInJump(uint32_t pos) {
	int16_t x = computeJump(pos + 2, code.size());
	auto it = &code[pos];
	writeI16(it, x);
}

Chunk::Chunk() : constants(new List()) {}

void Chunk::writeToFile(std::ofstream& fs) {
	fs.write((const char*) magicBytes.data(), magicBytes.size());
	
	std::ostream_iterator<char> it(fs);
	
	if(constants->vec.size() > 0xffff)
		throw std::runtime_error("Too many constants in program");
	uint16_t constantCnt = (uint16_t) constants->vec.size();
	writeUI16(it, constantCnt);
	
	for(Value cnst : constants->vec) {
		writeConstantToFile(it, cnst);
	}
	
	for(std::unique_ptr<FunctionChunk>& func : functions) {
		if(func->code.size() > 0xffff)
			throw std::runtime_error("Function too large");
		uint16_t codeSize = (uint16_t) func->code.size();
		writeUI16(it, codeSize);
		fs.write((const char*) func->code.data(), codeSize);
	}
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
	
	while(fs.peek() != EOF) {
		chunk->functions.emplace_back(new FunctionChunk());
		uint16_t codeSize = readUI16(it);
		chunk->functions.back()->code.resize(codeSize);
		fs.read((char*) chunk->functions.back()->code.data(), codeSize);
	}
	
	return chunk;
}

std::string Chunk::list() {
	std::stringstream res;
	
	res << "Constants:\n";
	for(uint32_t i = 0; i < constants->vec.size(); i++) {
		res << i << ": " << constants->vec[i].toString() << "\n";
	}
	res << "\n";
	
	for(uint32_t i = 0; i < functions.size(); i++) {
		res << "Function prototype " + std::to_string(i) + ":\n";
		auto it = functions[i]->code.begin();
		while(it != functions[i]->code.end()) {
			Opcode op = static_cast<Opcode>(readUI8(it));
			res << opcodeDesc(op);
			switch(op) {
			case Opcode::MAKE_FUNC: {
				res << " proto = " << (int) readUI16(it) << "; argCnt = " << (int) readUI16(it) << "\n  upvalues = [";
				uint16_t upvalues = readUI16(it);
				for(uint16_t i = 0; i < upvalues; i++) {
					res << (int) readI16(it);
					if(i != upvalues-1)
						res << ",";
				}
				res << "]";
				break;
			} case Opcode::LOCAL:
			case Opcode::SET_LOCAL:
				res << " " << (int) readI16(it);
				break;
			case Opcode::CONSTANT:
			case Opcode::GLOBAL:
			case Opcode::POP:
			case Opcode::CALL:
				res << " " << (int) readUI16(it);
				break;
			case Opcode::JUMP_IF_NOT:
			case Opcode::JUMP:
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
			default:
				break;
			}
			res << "\n";
		}
		res << "\n";
	}
	
	return res.str();
}
