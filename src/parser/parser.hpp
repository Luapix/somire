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
	
	std::unique_ptr<Node> parseProgram();
	
private:
	Lexer<C> lexer;
	
	std::unique_ptr<Node> curToken;
	std::unique_ptr<Node> peekToken;
	
	void error(std::string cause);
	
	std::unique_ptr<Node> nextToken();
	void discardToken(NodeType type);
	void discardSymbol(std::string sym);
	bool isCurSymbol(std::string sym);
	
	int getInfixPrecedence();
	std::unique_ptr<Node> parseType();
	std::unique_ptr<Node> parseFunction();
	std::unique_ptr<Node> parseExpr(int prec = 0);
	std::unique_ptr<Node> parseMultilineExpr();
	void finishStatement();
	std::unique_ptr<Node> parseIfStatement();
	std::unique_ptr<Node> parseStatement();
	std::unique_ptr<Node> parseBlock();
	std::unique_ptr<Node> parseIndentedBlock();
};

Parser<std::istreambuf_iterator<char>> newFileParser(std::ifstream& fs);

#include "parser.tpp"
