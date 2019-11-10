#pragma once

#include <vector>
#include <cstdint>
#include <memory>
#include <unordered_map>

#include "gc.hpp"
#include "value.hpp"

constexpr std::array<uint8_t, 8> magicBytes = { 'S','o','m','i','r','&', 0, 1 };

enum class Opcode : uint8_t {
	IGNORE,
	CONSTANT,
	UNI_MINUS,
	BIN_PLUS
};

std::string opcodeDesc(Opcode opcode);

std::array<uint8_t, 4> serializeUInt(uint32_t x);
uint32_t parseUInt(std::array<uint8_t, 4> b);

std::array<uint8_t, 8> serializeReal(double x);
double parseReal(std::array<uint8_t, 8> b);

class Chunk {
public:
	GC::Root<List> constants;
	std::vector<uint8_t> bytecode;
	
	Chunk();
	
	template <typename O>
	void writeToFile(O& output);
	
	template <typename I>
	static std::unique_ptr<Chunk> loadFromFile(I& input);
	
private:
	template <typename O>
	void writeConstantToFile(O& output, Value val);
	
	template <typename I>
	void loadConstantFromFile(I& input);
};

#include "chunk.tpp"
