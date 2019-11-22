#ifndef PARSER_IMPL_HPP
#define PARSER_IMPL_HPP

#include <fstream>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include "utf8.h"


template<typename C>
Parser<C>::Parser(C start, C end) : lexer(start, end) {
	nextToken(); nextToken();
}

template<typename C>
[[noreturn]] void Parser<C>::error(std::string cause) {
	lexer.error(cause, true);
}

template<typename C>
std::unique_ptr<Node> Parser<C>::nextToken() {
	std::unique_ptr<Node> token = lexer.lexToken(); // Get new token
	curToken.swap(peekToken);
	peekToken.swap(token);
	return token; // Return old token
}

template<typename C>
void Parser<C>::discardToken(NodeType type) {
	std::unique_ptr<Node> token = nextToken();
	if(token->type != type)
		error("Expected " + nodeTypeDesc(type) + ", got " + token->toString());
}

template<typename C>
void Parser<C>::discardSymbol(std::string sym) {
	if(!isCurSymbol(sym))
		error("Expected symbol '" + sym + "', got " + curToken->toString());
	nextToken();
}

template<typename C>
bool Parser<C>::isCurSymbol(std::string sym) {
	if(curToken->type != NodeType::SYM) return false;
	NodeSymbol* symToken = static_cast<NodeSymbol*>(curToken.get());
	return symToken->val == sym;
}

std::unordered_set<NodeType> terminals = { NodeType::ID, NodeType::INT, NodeType::REAL, NodeType::STR };
std::unordered_set<std::string> terminalSymbols = { "nil", "true", "false" };
std::unordered_set<std::string> prefixOperators = { "+", "-", "not" };
std::unordered_set<std::string> infixOperators = { "+", "-", "*", "/", "^", "%", "and", "or", "(", "==", "!=", ">", "<", ">=", "<=" };
std::unordered_set<std::string> rightAssociativeOperators = { "^" };

std::unordered_map<std::string, int> operatorPrecedence = {
	{"and", 2}, {"or", 2},
	{"not", 4},
	{"==", 6},  {"!=", 6},
	{">", 6},   {"<", 6},  {">=", 6}, {"<=", 6},
	{"+", 8},   {"-", 8},
	{"*", 10},  {"/", 10}, {"%", 10},
	{"^", 12},
	{"(", 14}
};

template<typename C>
int Parser<C>::getInfixPrecedence() {
	if(curToken->type == NodeType::SYM) {
		std::string sym = static_cast<NodeSymbol*>(curToken.get())->val;
		if(infixOperators.find(sym) != infixOperators.end()) {
			return operatorPrecedence[sym];
		}
	}
	return -1; // Not an infix/postfix operator
}

template<typename C>
std::unique_ptr<Node> Parser<C>::parseExpr(int prec) {
	std::unique_ptr<Node> exp;
	if(terminals.find(curToken->type) != terminals.end()) {
		exp = nextToken();
	} else if(curToken->type == NodeType::SYM) {
		std::unique_ptr<NodeSymbol> symbol(static_cast<NodeSymbol*>(nextToken().release()));
		if(terminalSymbols.find(symbol->val) != terminalSymbols.end()) {
			exp = std::move(symbol);
		} else if(prefixOperators.find(symbol->val) != prefixOperators.end()) {
			int prec2 = operatorPrecedence[symbol->val];
			exp = std::unique_ptr<Node>(new NodeUnary(symbol->val, parseExpr(prec2)));
		} else if(symbol->val == "(") {
			exp = parseExpr(0);
			discardSymbol(")");
		} else if(symbol->val == "[") {
			std::vector<std::unique_ptr<Node>> vals;
			if(!isCurSymbol("]")) {
				while(true) {
					vals.push_back(parseExpr(0));
					if(isCurSymbol("]"))
						break;
					else
						discardSymbol(",");
				}
			}
			nextToken();
			exp = std::unique_ptr<Node>(new NodeList(std::move(vals)));
		} else {
			error("Unexpected symbol at start of expression: " + symbol->val);
		}
	} else {
		error("Unexpected token at start of expression: " + curToken->toString());
	}
	
	while(getInfixPrecedence() > prec) {
		std::unique_ptr<NodeSymbol> symbol(static_cast<NodeSymbol*>(nextToken().release()));
		if(infixOperators.find(symbol->val) != infixOperators.end()) {
			if(symbol->val == "(") {
				std::vector<std::unique_ptr<Node>> args;
				if(!isCurSymbol(")")) {
					while(true) {
						args.push_back(parseExpr(0));
						if(isCurSymbol(")"))
							break;
						else
							discardSymbol(",");
					}
				}
				nextToken();
				exp = std::unique_ptr<Node>(new NodeCall(std::move(exp), std::move(args)));
			} else {
				int prec2 = operatorPrecedence[symbol->val];
				if(rightAssociativeOperators.find(symbol->val) != rightAssociativeOperators.end())
					prec2--;
				exp = std::unique_ptr<Node>(new NodeBinary(symbol->val, std::move(exp), parseExpr(prec2)));
			}
		} else {
			error("Unimplemented infix/postfix operator: " + symbol->val);
		}
	}
	
	return exp;
}

template<typename C>
std::unique_ptr<Node> Parser<C>::parseMultilineExpr() {
	std::unique_ptr<Node> exp;
	if(isCurSymbol("fun")) {
		nextToken();
		discardSymbol("(");
		std::vector<std::string> argNames;
		if(!isCurSymbol(")")) {
			while(true) {
				if(curToken->type != NodeType::ID)
					error("Expected identifier in argument list, got " + nodeTypeDesc(curToken->type));
				std::unique_ptr<Node> argToken = nextToken();
				argNames.push_back(static_cast<NodeId*>(argToken.get())->val);
				if(isCurSymbol(")"))
					break;
				else
					discardSymbol(",");
			}
		}
		nextToken();
		discardSymbol(":");
		exp = std::unique_ptr<Node>(new NodeFunction(argNames, parseIndentedBlock()));
	} else {
		exp = parseExpr();
		finishStatement();
	}
	return exp;
}

template<typename C>
void Parser<C>::finishStatement() {
	if(!(curToken->type == NodeType::EOI || curToken->type == NodeType::DEDENT))
		discardToken(NodeType::NL);
}

template<typename C>
std::unique_ptr<Node> Parser<C>::parseIfStatement() {
	nextToken();
	std::unique_ptr<Node> cond = parseExpr();
	discardSymbol(":");
	std::unique_ptr<NodeIf> node(new NodeIf(std::move(cond), parseIndentedBlock()));
	if(isCurSymbol("else")) {
		nextToken();
		if(isCurSymbol("if")) {
			std::vector<std::unique_ptr<Node>> statements;
			statements.push_back(parseIfStatement());
			node->elseBlock = std::unique_ptr<Node>(new NodeBlock(std::move(statements)));
		} else {
			discardSymbol(":");
			node->elseBlock = parseIndentedBlock();
		}
	}
	return node;
}

template<typename C>
std::unique_ptr<Node> Parser<C>::parseStatement() {
	if(isCurSymbol("let")) {
		nextToken();
		std::unique_ptr<Node> idToken = nextToken();
		if(idToken->type != NodeType::ID)
			error("Expected identifier after 'let', got " + nodeTypeDesc(idToken->type));
		std::string id = static_cast<NodeId*>(idToken.get())->val;
		std::unique_ptr<Node> expr;
		if(isCurSymbol("(")) {
			nextToken();
			std::vector<std::string> argNames;
			if(!isCurSymbol(")")) {
				while(true) {
					if(curToken->type != NodeType::ID)
						error("Expected identifier in argument list, got " + nodeTypeDesc(curToken->type));
					std::unique_ptr<Node> argToken = nextToken();
					argNames.push_back(static_cast<NodeId*>(argToken.get())->val);
					if(isCurSymbol(")"))
						break;
					else
						discardSymbol(",");
				}
			}
			nextToken();
			discardSymbol(":");
			expr = std::unique_ptr<Node>(new NodeFunction(argNames, parseIndentedBlock()));
		} else {
			discardSymbol("=");
			expr = parseMultilineExpr();
		}
		return std::unique_ptr<Node>(new NodeLet(id, std::move(expr)));
	} else if(curToken->type == NodeType::ID && peekToken->type == NodeType::SYM && static_cast<NodeSymbol&>(*peekToken).val == "=") {
		std::unique_ptr<Node> idToken = nextToken();
		std::string id = static_cast<NodeId*>(idToken.get())->val;
		nextToken();
		std::unique_ptr<Node> expr = parseMultilineExpr();
		return std::unique_ptr<Node>(new NodeSet(id, std::move(expr)));
	} else if(isCurSymbol("if")) {
		return parseIfStatement();
	} else if(isCurSymbol("while")) {
		nextToken();
		std::unique_ptr<Node> cond = parseExpr();
		if(!isCurSymbol(":"))
			error("Expected ':' after 'while', got " + curToken->toString());
		nextToken();
		std::unique_ptr<Node> block = parseIndentedBlock();
		return std::unique_ptr<Node>(new NodeWhile(std::move(cond), std::move(block)));
	} else if(isCurSymbol("return")) {
		nextToken();
		std::unique_ptr<Node> expr = parseMultilineExpr();
		return std::unique_ptr<Node>(new NodeReturn(std::move(expr)));
	} else {
		std::unique_ptr<Node> expr = parseMultilineExpr();
		return std::unique_ptr<Node>(new NodeExprStat(std::move(expr)));
	}
}

template<typename C>
std::unique_ptr<Node> Parser<C>::parseBlock() {
	std::vector<std::unique_ptr<Node>> statements;
	while(curToken->type != NodeType::DEDENT && curToken->type != NodeType::EOI) {
		statements.push_back(parseStatement());
	}
	return std::unique_ptr<NodeBlock>(new NodeBlock(std::move(statements)));
}

template<typename C>
std::unique_ptr<Node> Parser<C>::parseIndentedBlock() {
	if(curToken->type != NodeType::INDENT)
		error("Expected indent, got " + curToken->toString());
	std::string oldIndent = static_cast<NodeIndent&>(*curToken).oldIndent;
	nextToken();
	std::unique_ptr<Node> block = parseBlock();
	if(curToken->type == NodeType::DEDENT) {
		std::string newIndent = static_cast<NodeDedent&>(*curToken).newIndent;
		if(newIndent == oldIndent) {
			nextToken();
		} else if(newIndent.substr(0, oldIndent.length()) == oldIndent) {
			// if newIndent starts with oldIndent but isn't equal, it's an invalid, partial dedent
			throw ParseError("Invalid dedent in block");
		}
	}
	return block;
}

template<typename C>
std::unique_ptr<Node> Parser<C>::parseProgram() {
	discardToken(NodeType::NL);
	std::unique_ptr<Node> program = parseBlock();
	discardToken(NodeType::EOI);
	return program;
}


Parser<std::istreambuf_iterator<char>> newFileParser(std::ifstream& fs) {
	if(!fs.is_open()) throw ParseError("Unable to open file");
	std::istreambuf_iterator<char> it(fs);
	std::istreambuf_iterator<char> end_it;
	return Parser<std::istreambuf_iterator<char>>(it, end_it);
}

#endif