#ifndef PARSER_HPP
#define PARSER_HPP

#include <cstdint>
#include <iostream>
#include <fstream>
#include <exception>

#include "utf8.h"
#include "uni_data.hpp"

#define UNI_EOI UINT32_MAX

class ParseError : public std::runtime_error {
public:
	ParseError(const std::string& what);
};

template <typename C>
class Parser {
public:
	Parser(C start, C end);
	
	void printAll();
	
private:
	C curByte;
	C end;
	uni_cp curChar;
	
	void next();
};

Parser<std::istreambuf_iterator<char>> newFileParser(std::ifstream& fs);

#include "parser_impl.hpp"

#endif