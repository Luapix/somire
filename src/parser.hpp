#ifndef PARSER_HPP
#define PARSER_HPP

#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <exception>
#include <iterator>
#include <memory>

#include "utf8.h"
#include "ast.hpp"
#include "uni_data.hpp"

#define UNI_EOI UINT32_MAX

template <typename C>
class Parser {
public:
	Parser(C start, C end);
	
	std::unique_ptr<Node> lexNewline();
	std::unique_ptr<Node> lexId();

private:
	C curByte;
	C end;
	uni_cp curChar;
	
	int curLine;
	std::string curIndent;
	
	void error(std::string cause);
	
	void next();
};

Parser<std::istreambuf_iterator<char>> newFileParser(std::ifstream& fs);

#include "parser_impl.hpp"

#endif