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
#include <iterator>
#include <unordered_set>

#include "utf8.h"

#include "ast.hpp"
#include "uni_data.hpp"
#include "uni_util.hpp"

#define UNI_EOI UINT32_MAX

template <typename C>
class Parser {
public:
	Parser(C start, C end);
	
	std::unique_ptr<Node> lexToken();
	
private:
	C curByte;
	C end;
	
	uni_cp curChar;
	uni_cp peekChar;
	uni_cp peekChar2;
	
	int curLine;
	std::string curIndent;
	
	void error(std::string cause);
	void next();
	void skipSpace(bool skipSpace = false);
	
	std::unique_ptr<Node> lexNewline();
	std::unique_ptr<Node> lexId();
	std::unique_ptr<Node> lexNumber();
	std::unique_ptr<Node> lexString();
	std::unique_ptr<Node> lexSymbol();
	
	std::unique_ptr<Node> expectToken(NodeType type);
};

Parser<std::istreambuf_iterator<char>> newFileParser(std::ifstream& fs);

#include "parser_impl.hpp"

#endif