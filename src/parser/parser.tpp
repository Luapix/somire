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
GC::Root<Node> Parser<C>::nextToken() {
	GC::Root<Node> token = lexer.lexToken(); // Get new token
	curToken.swap(peekToken);
	peekToken.swap(token);
	return token; // Return old token
}

template<typename C>
void Parser<C>::discardToken(NodeType type) {
	GC::Root<Node> token = nextToken();
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
std::unordered_set<std::string> infixOperators = { "+", "-", "*", "/", "^", "%", "and", "or", "(", "==", "!=", ">", "<", ">=", "<=", "[", "." };
std::unordered_set<std::string> rightAssociativeOperators = { "^" };

std::unordered_map<std::string, int> operatorPrecedence = {
	{"and", 2}, {"or", 2},
	{"not", 4},
	{"==", 6},  {"!=", 6},
	{">", 6},   {"<", 6},  {">=", 6}, {"<=", 6},
	{"+", 8},   {"-", 8},
	{"*", 10},  {"/", 10}, {"%", 10},
	{"^", 12},
	{"(", 14}, {"[", 14}, {".", 14}
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
GC::Root<Node> Parser<C>::parseType() {
	if(curToken->type != NodeType::ID)
		error("Expected identifier in type constraint, got " + nodeTypeDesc(curToken->type));
	GC::Root<Node> idToken = nextToken();
	return GC::Root<Node>(new NodeSimpleType(static_cast<NodeId*>(idToken.get())->val));
}

template<typename C>
GC::Root<NodeExp> Parser<C>::parseFunction() {
	discardSymbol("(");
	std::vector<std::string> argNames;
	auto argTypes = GC::makeRootVector<Node>();
	if(!isCurSymbol(")")) {
		while(true) {
			if(curToken->type != NodeType::ID)
				error("Expected identifier in argument list, got " + nodeTypeDesc(curToken->type));
			GC::Root<Node> argToken = nextToken();
			argNames.push_back(static_cast<NodeId*>(argToken.get())->val);
			if(isCurSymbol(":")) {
				nextToken();
				argTypes->vec.push_back(parseType().release());
			} else {
				argTypes->vec.push_back(new NodeSimpleType("any"));
			}
			if(isCurSymbol(")")) {
				break;
			}
			discardSymbol(",");
		}
	}
	nextToken();
	discardSymbol(":");
	return GC::Root<NodeExp>(new NodeFunction(argNames, argTypes->vec, parseIndentedBlock().release()));
}

template<typename C>
GC::Root<NodeExp> Parser<C>::parseExpr(int prec) {
	GC::Root<NodeExp> exp;
	if(terminals.find(curToken->type) != terminals.end()) {
		exp.reset(static_cast<NodeExp*>(nextToken().release()));
	} else if(curToken->type == NodeType::SYM) {
		GC::Root<NodeSymbol> symbol(static_cast<NodeSymbol*>(nextToken().release()));
		if(terminalSymbols.find(symbol->val) != terminalSymbols.end()) {
			exp.reset(symbol.release());
		} else if(prefixOperators.find(symbol->val) != prefixOperators.end()) {
			int prec2 = operatorPrecedence[symbol->val];
			exp.reset(new NodeUnary(symbol->val, parseExpr(prec2).release()));
		} else if(symbol->val == "(") {
			exp = parseExpr(0);
			discardSymbol(")");
		} else if(symbol->val == "[") {
			auto vals = GC::makeRootVector<NodeExp>();
			if(!isCurSymbol("]")) {
				while(true) {
					vals->vec.push_back(parseExpr(0).release());
					if(isCurSymbol("]"))
						break;
					else
						discardSymbol(",");
				}
			}
			nextToken();
			exp.reset(new NodeList(vals->vec));
		} else {
			error("Unexpected symbol at start of expression: " + symbol->val);
		}
	} else {
		error("Unexpected token at start of expression: " + curToken->toString());
	}
	
	while(getInfixPrecedence() > prec) {
		GC::Root<NodeSymbol> symbol(static_cast<NodeSymbol*>(nextToken().release()));
		if(infixOperators.find(symbol->val) != infixOperators.end()) {
			if(symbol->val == "(") {
				auto args = GC::makeRootVector<NodeExp>();
				if(!isCurSymbol(")")) {
					while(true) {
						args->vec.push_back(parseExpr(0).release());
						if(isCurSymbol(")"))
							break;
						else
							discardSymbol(",");
					}
				}
				nextToken();
				exp.reset(new NodeCall(exp.release(), args->vec));
			} else if(symbol->val == "[") {
				exp.reset(new NodeBinary("index", exp.release(), parseExpr(0).release()));
				discardSymbol("]");
			} else if(symbol->val == ".") {
				if(curToken->type != NodeType::ID)
					error("Expected id after '.', got " + curToken->toString());
				GC::Root<NodeId> prop(static_cast<NodeId*>(nextToken().release()));
				exp.reset(new NodeProp(exp.release(), prop->val));
			} else {
				int prec2 = operatorPrecedence[symbol->val];
				if(rightAssociativeOperators.find(symbol->val) != rightAssociativeOperators.end())
					prec2--;
				exp.reset(new NodeBinary(symbol->val, exp.release(), parseExpr(prec2).release()));
			}
		} else {
			error("Unimplemented infix/postfix operator: " + symbol->val);
		}
	}
	
	return exp;
}

template<typename C>
GC::Root<NodeExp> Parser<C>::parseMultilineExpr() {
	GC::Root<NodeExp> exp;
	if(isCurSymbol("fun")) {
		nextToken();
		exp = parseFunction();
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
GC::Root<Node> Parser<C>::parseIfStatement() {
	nextToken();
	GC::Root<NodeExp> cond = parseExpr();
	discardSymbol(":");
	GC::Root<NodeIf> node(new NodeIf(cond.release(), parseIndentedBlock().release()));
	if(isCurSymbol("else")) {
		nextToken();
		if(isCurSymbol("if")) {
			auto statements = GC::makeRootVector<Node>();
			statements->vec.push_back(parseIfStatement().release());
			node->elseBlock = new NodeBlock(statements->vec);
		} else {
			discardSymbol(":");
			node->elseBlock = parseIndentedBlock().release();
		}
	}
	return GC::Root<Node>(node.release());
}

template<typename C>
GC::Root<Node> Parser<C>::parseStatement() {
	if(isCurSymbol("let")) {
		nextToken();
		GC::Root<Node> idToken = nextToken();
		if(idToken->type != NodeType::ID)
			error("Expected identifier after 'let', got " + nodeTypeDesc(idToken->type));
		std::string id = static_cast<NodeId*>(idToken.get())->val;
		GC::Root<NodeExp> expr;
		if(isCurSymbol("(")) {
			expr = parseFunction();
		} else {
			discardSymbol("=");
			expr = parseMultilineExpr();
		}
		return GC::Root<Node>(new NodeLet(id, expr.release()));
	} else if(curToken->type == NodeType::ID && peekToken->type == NodeType::SYM && static_cast<NodeSymbol&>(*peekToken).val == "=") {
		GC::Root<Node> idToken = nextToken();
		std::string id = static_cast<NodeId*>(idToken.get())->val;
		nextToken();
		GC::Root<NodeExp> expr = parseMultilineExpr();
		return GC::Root<Node>(new NodeSet(id, expr.release()));
	} else if(isCurSymbol("if")) {
		return parseIfStatement();
	} else if(isCurSymbol("while")) {
		nextToken();
		GC::Root<NodeExp> cond = parseExpr();
		if(!isCurSymbol(":"))
			error("Expected ':' after 'while', got " + curToken->toString());
		nextToken();
		GC::Root<Node> block = parseIndentedBlock();
		return GC::Root<Node>(new NodeWhile(cond.release(), block.release()));
	} else if(isCurSymbol("return")) {
		nextToken();
		GC::Root<NodeExp> expr = parseMultilineExpr();
		return GC::Root<Node>(new NodeReturn(expr.release()));
	} else {
		GC::Root<NodeExp> expr = parseMultilineExpr();
		return GC::Root<Node>(new NodeExprStat(expr.release()));
	}
}

template<typename C>
GC::Root<Node> Parser<C>::parseBlock() {
	auto statements = GC::makeRootVector<Node>();
	while(curToken->type != NodeType::DEDENT && curToken->type != NodeType::EOI) {
		statements->vec.push_back(parseStatement().release());
	}
	return GC::Root<Node>(new NodeBlock(statements->vec));
}

template<typename C>
GC::Root<Node> Parser<C>::parseIndentedBlock() {
	if(curToken->type != NodeType::INDENT)
		error("Expected indent, got " + curToken->toString());
	std::string oldIndent = static_cast<NodeIndent&>(*curToken).oldIndent;
	nextToken();
	GC::Root<Node> block = parseBlock();
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
GC::Root<Node> Parser<C>::parseProgram() {
	discardToken(NodeType::NL);
	GC::Root<Node> program = parseBlock();
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