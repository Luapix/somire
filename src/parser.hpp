#ifndef PARSER_HPP
#define PARSER_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <climits>
#include <exception>
#include <stdexcept>
#include <cassert>
#include <memory>

#include <iostream>

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
	uni_cp peekChar;
	
	int curLine;
	std::string curIndent;
	
	void error(std::string cause);
	
	void next();
	uni_cp peek();
	void skipSpace(bool skipSpace = false);
	
	std::unique_ptr<Node> lexNewline();
	
	std::unique_ptr<Node> lexId();
	std::unique_ptr<Node> lexNumber();
	std::unique_ptr<Node> lexExpr();
};

Parser<std::istreambuf_iterator<char>> newFileParser(std::ifstream& fs);

#include "parser_impl.hpp"

#endif