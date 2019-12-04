#pragma once

#include <string>
#include <memory>

#include "util/gc.hpp"
#include "util/uni_data.hpp"
#include "ast.hpp"

#define UNI_EOI UINT32_MAX

template<typename C>
class Lexer {
public:
	Lexer(C start, C end);
	
	void error(std::string cause, bool parseError = false);
	
	GC::Root<Node> lexToken();
	
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
	
	GC::Root<Node> lexNewline();
	GC::Root<Node> lexId();
	GC::Root<Node> lexNumber();
	GC::Root<Node> lexString();
	GC::Root<Node> lexSymbol();
};

#include "lexer.tpp"
