#include "chunk.hpp"

#include <stdexcept>

std::array<uint8_t, 4> serializeUInt(uint32_t x) {
	std::array<uint8_t, 4> res;
	res[0] = x;
	res[1] = x >> 8;
	res[2] = x >> 16;
	res[3] = x >> 24;
	return res;
}

uint32_t parseUInt(std::array<uint8_t, 4> b) {
	uint32_t u0 = b[0], u1 = b[1], u2 = b[2], u3 = b[3];
    return u0 | (u1 << 8) | (u2 << 16) | (u3 << 24);
}

Chunk::Chunk() {}
