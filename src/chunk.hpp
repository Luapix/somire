#pragma once

#include <vector>
#include <cstdint>
#include <memory>

constexpr std::array<uint8_t, 8> magicBytes = { 'S','o','m','i','r','&', 0, 1 };

class Chunk {
public:
	Chunk();
	
	template <typename O>
	void writeToFile(O& output);
	
	template <typename I>
	static std::unique_ptr<Chunk> loadFromFile(I& input);
	
private:
	std::vector<uint8_t> bytecode;
};

#include "chunk.tpp"
