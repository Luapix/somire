#pragma once

#include <vector>
#include <cstdint>
#include <memory>
#include <unordered_map>

constexpr std::array<uint8_t, 8> magicBytes = { 'S','o','m','i','r','&', 0, 1 };

enum Opcode : uint8_t {
	NO_OP
};

class Chunk {
public:
	std::vector<uint8_t> bytecode;
	
	Chunk();
	
	template <typename O>
	void writeToFile(O& output);
	
	template <typename I>
	static std::unique_ptr<Chunk> loadFromFile(I& input);
};

#include "chunk.tpp"
