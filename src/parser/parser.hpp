#pragma once

#include <string>
#include <memory>
#include <iterator>

#include "ast.hpp"
#include "lexer.hpp"

#define UNI_EOI UINT32_MAX

template<typename C>
class Parser {
public:
	Parser(C start, C end);
	
	GC::Root<Node> parseProgram();
	
private:
	Lexer<C> lexer;
	
	GC::Root<Node> curToken;
	GC::Root<Node> peekToken;
	
	void error(std::string cause);
	
	GC::Root<Node> nextToken();
	void discardToken(NodeType type);
	void discardSymbol(std::string sym);
	bool isCurSymbol(std::string sym);
	
	int getInfixPrecedence();
	GC::Root<Node> parseType();
	GC::Root<NodeExp> parseFunction();
	GC::Root<NodeExp> parseExpr(int prec = 0);
	GC::Root<NodeExp> parseMultilineExpr();
	void finishStatement();
	GC::Root<Node> parseIfStatement();
	GC::Root<Node> parseStatement();
	GC::Root<Node> parseBlock();
	GC::Root<Node> parseIndentedBlock();
};

Parser<std::istreambuf_iterator<char>> newFileParser(std::ifstream& fs);

#include "parser.tpp"
