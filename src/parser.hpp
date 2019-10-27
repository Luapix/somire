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
#include <unordered_map>
#include <vector>

#include "utf8.h"

#include "ast.hpp"
#include "uni_data.hpp"
#include "uni_util.hpp"

#define UNI_EOI UINT32_MAX

template <typename C>
class Parser {
public:
	Parser(C start, C end);
	
	std::unique_ptr<Node> testParse();
	
private:
	C curByte;
	C end;
	
	uni_cp curChar;
	uni_cp peekChar;
	uni_cp peekChar2;
	
	int curLine;
	std::string curIndent;
	
	void error(std::string cause);
	void nextChar();
	void skipSpace(bool skipSpace = false);
	
	std::unique_ptr<Node> lexNewline();
	std::unique_ptr<Node> lexId();
	std::unique_ptr<Node> lexNumber();
	std::unique_ptr<Node> lexString();
	std::unique_ptr<Node> lexSymbol();
	std::unique_ptr<Node> lexToken();
	
	std::unique_ptr<Node> curToken;
	
	std::unique_ptr<Node> nextToken();
	void discardToken(NodeType type);
	bool isCurSymbol(std::string sym);
	
	int getInfixPrecedence();
	std::unique_ptr<Node> parseExpr(int prec = 0);
	std::unique_ptr<Node> parseStatement();
	std::unique_ptr<NodeProgram> parseProgram();
};

Parser<std::istreambuf_iterator<char>> newFileParser(std::ifstream& fs);

#include "parser_impl.hpp"

#endif