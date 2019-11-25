#pragma once

#include <ios>
#include <iostream>
#include <algorithm>

template<typename O>
void writeUI8(O& it, uint8_t x) {
	*it++ = x;
}

template<typename O>
void writeUI16(O& it, uint16_t x) {
	for(uint8_t i = 0; i < 2; i++) {
		*it++ = (uint8_t) (x >> (i*8));
	}
}

template<typename O>
void writeI16(O& it, int16_t x) {
	writeUI16(it, (uint16_t) x);
}

template<typename O>
void writeUI32(O& it, uint32_t x) {
	for(uint8_t i = 0; i < 4; i++) {
		*it++ = (uint8_t) (x >> (i*8));
	}
}

template<typename O>
void writeI32(O& it, int32_t x) {
	writeUI32(it, (uint32_t) x);
}

template<typename O>
void writeDouble(O& it, double x) {
	uint64_t& x2 = reinterpret_cast<uint64_t&>(x);
	for(uint8_t i = 0; i < 8; i++) {
		*it++ = (uint8_t) (x2 >> (i*8));
	}
}

template<typename I>
uint8_t readUI8(I& it) {
	return *it++;
}

template<typename I>
uint16_t readUI16(I& it) {
	uint16_t x = 0;
	for(uint8_t i = 0; i < 2; i++) {
		x |= ((uint16_t) ((uint8_t) *it++)) << (i*8);
	}
	return x;
}

template<typename I>
int16_t readI16(I& it) {
	return (int16_t) readUI16(it);
}

template<typename I>
uint32_t readUI32(I& it) {
	uint32_t x = 0;
	for(uint8_t i = 0; i < 4; i++) {
		x |= ((uint32_t) ((uint8_t) *it++)) << (i*8);
	}
	return x;
}

template<typename I>
int32_t readI32(I& it) {
	return (int32_t) readUI32(it);
}

template<typename I>
double readDouble(I& it) {
	uint64_t x = 0;
	for(uint8_t i = 0; i < 8; i++) {
		x |= ((uint64_t) ((uint8_t) *it++)) << (i*8);
	}
	return reinterpret_cast<double&>(x);
}

template <typename O>
void Chunk::writeConstantToFile(O& it, Value val) {
	if(val.isNil()) {
		writeUI8(it, (uint8_t) ConstantType::NIL);
	} else if(val.isBool()) {
		writeUI8(it, (uint8_t) ConstantType::BOOL);
		writeUI8(it, (uint8_t) val.getBool());
	} else if(val.isInt()) {
		writeUI8(it, (uint8_t) ConstantType::INT);
		writeI32(it, val.getInt());
	} else if(val.isReal()) {
		writeUI8(it, (uint8_t) ConstantType::REAL);
		writeDouble(it, val.getReal());
	} else {
		String* strPointer;
		if(val.isObject() && (strPointer = val.get<String>())) {
			std::string& str = strPointer->str;
			writeUI8(it, (uint8_t) ConstantType::STR);
			writeUI32(it, (uint32_t) str.size());
			std::copy(str.begin(), str.end(), it);
		} else {
			throw std::runtime_error("Type " + val.getTypeDesc() + " cannot be a constant");
		}
	}
}

template <typename I>
void Chunk::loadConstantFromFile(I& it) {
	ConstantType type = (ConstantType) readUI8(it);
	
	switch(type) {
	case ConstantType::NIL:
		constants->vec.push_back(Value::nil());
		break;
	case ConstantType::BOOL:
		constants->vec.push_back(Value((bool) readUI8(it)));
		break;
	case ConstantType::INT:
		constants->vec.emplace_back(readI32(it));
		break;
	case ConstantType::REAL:
		constants->vec.emplace_back(readDouble(it));
		break;
	case ConstantType::STR: {
		uint32_t len = readUI32(it);
		std::string str(len, '\0');
		std::copy_n(it, len, str.begin());
		if(len > 0) ++it;
		constants->vec.emplace_back(new String(str));
		break;
	}}
}
