#pragma once

#include <string>
#include <memory>

#include "uni_data.hpp"
#include "ast.hpp"

#define UNI_EOI UINT32_MAX

template<typename C>
class Lexer {
public:
	Lexer(C start, C end);
	
	void error(std::string cause, bool parseError = false);
	
	std::unique_ptr<Node> lexToken();
	
private:
	C curByte;
	C end;
	
	uni_cp curChar;
	uni_cp peekChar;
	uni_cp peekChar2;
	
	int curLine;
	std::string curIndent;
	
	void nextChar();
	void skipSpace(bool skipSpace = false);
	
	std::unique_ptr<Node> lexNewline();
	std::unique_ptr<Node> lexId();
	std::unique_ptr<Node> lexNumber();
	std::unique_ptr<Node> lexString();
	std::unique_ptr<Node> lexSymbol();
};

#include "lexer.tpp"
