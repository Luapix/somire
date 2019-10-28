#pragma once

#include <ios>

template <typename O>
void Chunk::writeToFile(O& output) {
	output.write((const char*) magicBytes.data(), magicBytes.size());
	output.write((const char*) bytecode.data(), bytecode.size());
}

template <typename I>
std::unique_ptr<Chunk> Chunk::loadFromFile(I& input) {
	std::unique_ptr<Chunk> chunk(new Chunk());
	
	std::array<uint8_t, magicBytes.size()> buffer;
	input.read((char*) buffer.data(), buffer.size());
	if((int) input.gcount() != (int) buffer.size()) {
		throw std::runtime_error("Unexpected EOF in bytecode file");
	}
	if(buffer != magicBytes) {
		throw std::runtime_error("Invalid Somiré bytecode file");
	}
	
	input.seekg(0, std::ios::end);
	size_t size = (size_t) input.tellg() - magicBytes.size();
	chunk->bytecode.reserve(size);
	
	input.seekg(magicBytes.size(), std::ios::beg);
	input.read((char*) chunk->bytecode.data(), size);
	
	return chunk;
}
