#pragma once

#include <vector>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <iterator>

#include "gc.hpp"
#include "value.hpp"

class CompileError : public std::runtime_error {
public:
	CompileError(const std::string& what);
};

constexpr std::array<uint8_t, 8> magicBytes = { 'S','o','m','i','r','&', 0, 1 };

enum class Opcode : uint8_t {
	IGNORE,
	CONSTANT,
	UNI_MINUS,
	BIN_PLUS, BIN_MINUS, MULTIPLY, DIVIDE,
	LET, POP, SET,
	LOCAL, GLOBAL,
	NOT, OR, AND,
	EQUALS,
	LESS, LESS_OR_EQ,
	JUMP_IF_NOT,
	JUMP,
	CALL,
	MAKE_FUNC
};

std::string opcodeDesc(Opcode opcode);

template<typename O>
void writeUI8(O& it, uint8_t x);
template<typename O>
void writeUI16(O& it, uint16_t x);
template<typename O>
void writeI16(O& it, int16_t x);
template<typename O>
void writeUI32(O& it, uint32_t x);
template<typename O>
void writeI32(O& it, int32_t x);
template<typename O>
void writeDouble(O& it, double x);

template<typename I>
uint8_t readUI8(I& it);
template<typename I>
uint16_t readUI16(I& it);
template<typename I>
int16_t readI16(I& it);
template<typename I>
uint32_t readUI32(I& it);
template<typename I>
int32_t readI32(I& it);
template<typename I>
double readDouble(I& it);

int16_t computeJump(uint32_t from, uint32_t to);

// /!\ Very touchy!
// For codeOut to remain valid, this class should not be copied,
// and code should not be reallocated outside of using codeOut.
class FunctionChunk {
public:
	std::vector<uint8_t> code;
	std::back_insert_iterator<std::vector<uint8_t>> codeOut;
	
	FunctionChunk();
	FunctionChunk(const FunctionChunk&) = delete;
	
	void fillInJump(uint32_t pos);
};

class Chunk {
public:
	GC::Root<List> constants;
	std::vector<std::unique_ptr<FunctionChunk>> functions;
	
	Chunk();
	
	void writeToFile(std::ofstream& fs);
	
	static std::unique_ptr<Chunk> loadFromFile(std::ifstream& fs);
	
	std::string list();
	
private:
	template <typename O>
	void writeConstantToFile(O& it, Value val);
	
	template <typename I>
	void loadConstantFromFile(I& it);
};

#include "chunk.tpp"
