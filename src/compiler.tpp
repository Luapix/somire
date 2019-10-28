#include <cstdint>
#include <array>

template <typename O>
void Compiler::writeProgram(O& output) {
	std::array<uint8_t, 8> magic = { 'S','o','m','i','r','&', 0, 1 };
	output.write((const char*) magic.data(), magic.size());
}
