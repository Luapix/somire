#ifndef PARSER_HPP
#define PARSER_HPP

#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <exception>
#include <memory>

#include "utf8.h"

#include "ast.hpp"
#include "uni_data.hpp"
#include "uni_util.hpp"

#define UNI_EOI UINT32_MAX

template <typename C>
class Parser {
public:
	Parser(C start, C end);
	
	std::unique_ptr<Node> lexProgram();
	
private:
	C curByte;
	C end;
	uni_cp curChar;
	
	int curLine;
	std::string curIndent;
	
	void error(std::string cause);
	
	void next();
	void skipSpace(bool skipSpace = false);
	
	std::unique_ptr<Node> lexNewline();
	std::unique_ptr<Node> lexId();
};

Parser<std::istreambuf_iterator<char>> newFileParser(std::ifstream& fs);

#include "parser_impl.hpp"

#endif