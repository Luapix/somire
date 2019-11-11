#ifndef PARSER_IMPL_HPP
#define PARSER_IMPL_HPP

#include <iostream>


template <typename C>
Parser<C>::Parser(C start, C end)
	: curByte(start), end(end), curLine(0), curIndent() {
	nextChar(); nextChar(); nextChar();
	nextToken(); nextToken();
}

template <typename C>
[[noreturn]] void Parser<C>::error(std::string cause) {
	throw ParseError("At line " + std::to_string(curLine) + ": " + cause);
}

template <typename C>
void Parser<C>::nextChar() {
	curChar = peekChar;
	peekChar = peekChar2;
	if(curByte == end) {
		peekChar2 = UNI_EOI;
	} else {
		peekChar2 = utf8::next(curByte, end);
	}
}

template <typename C>
void Parser<C>::skipSpace(bool allowNL) {
	while(isSpace(curChar)) {
		if(curChar == '\n') {
			if(!allowNL) return;
			lexNewline();
			return;
		} else {
			nextChar();
		}
	}
}

template <typename C>
std::unique_ptr<Node> Parser<C>::lexNewline() {
	std::string newIndent;
	while(isSpace(curChar)) {
		if(curChar == '\n') {
			curLine++;
			newIndent.clear();
		} else {
			appendCP(newIndent, curChar);
		}
		nextChar();
	}
	std::unique_ptr<Node> newline;
	if(newIndent == curIndent) {
		newline.reset(new Node(NodeType::NL));
	} else if(newIndent.substr(0, curIndent.length()) == curIndent) {
		newline.reset(new NodeIndent(curIndent));
	} else if(curIndent.substr(0, newIndent.length()) == newIndent) {
		newline.reset(new NodeDedent(newIndent));
	} else {
		error("Invalid indentation");
	}
	curIndent = newIndent;
	return newline;
}

std::unordered_set<std::string> keywords = {
	"log", "let", "if",
	"not", "and", "or",
	"nil", "true", "false"
};

template <typename C>
std::unique_ptr<Node> Parser<C>::lexId() {
	std::string val = strFromCP(curChar);
	nextChar();
	while(isIdContinue(curChar)) {
		appendCP(val, curChar);
		nextChar();
	}
	if(keywords.find(val) != keywords.end()) {
		return std::unique_ptr<Node>(new NodeSymbol(val));
	} else {
		return std::unique_ptr<Node>(new NodeId(val));
	}
}

#if INT_MAX != INT32_MAX
	#error Can''t be bothered to deal with 16-bit ints
#endif
template <typename C>
std::unique_ptr<Node> Parser<C>::lexNumber() {
	std::string s;
	unsigned int base = 10;
	bool isReal = false;
	if(curChar == '0') { // check for a base specifier
		uni_cp b = peekChar;
		if(b == 'x' || b == 'o' || b == 'b') {
			if(b == 'x') base = 16;
			else if(b == 'o') base = 8;
			else if(b == 'b') base = 2;
			if(!isDigit(peekChar2, base)) { // the literal stops at 0
				nextChar();
				return std::unique_ptr<Node>(new NodeInt(0));
			}
			nextChar(); nextChar(); // skip base specifier
		}
	}
	while(isDigit(curChar, base)) {
		appendCP(s, curChar); nextChar();
		if(curChar == '.' && !isReal && s.size() > 0 && base == 10 && isDigit(peekChar, base)) {
			isReal = true;
			appendCP(s, curChar);
			nextChar();
		} else if((curChar == 'e' || curChar == 'E') && (isReal || (!isReal && base == 10))) {
			bool hasSign = (peekChar == '+' || peekChar == '-') && isDigit(peekChar2, 10);
			bool isExponent = isDigit(peekChar, 10) || hasSign;
			if(isExponent) {
				isReal = true;
				appendCP(s, curChar); nextChar();
				if(hasSign) { appendCP(s, curChar); nextChar(); }
				while(isDigit(curChar, 10)) {
					appendCP(s, curChar); nextChar();
				}
			}
			break;
		} 
	}
	if(isReal) {
		size_t end;
		double val;
		try {
			val = std::stod(s, &end);
		} catch(std::out_of_range& ex) {
			error("Real literal too large");
		}
		assert(end == s.size());
		return std::unique_ptr<Node>(new NodeReal(val));
	} else {
		size_t end;
		int val;
		try {
			val = std::stoi(s, &end, base);
		} catch(std::out_of_range& ex) {
			error("Integer literal too large");
		}
		assert(end == s.size());
		std::int32_t val2 = static_cast<std::int32_t>(val);
		return std::unique_ptr<Node>(new NodeInt(val2));
	}
}

template <typename C>
std::unique_ptr<Node> Parser<C>::lexString() {
	uni_cp delimiter = curChar;
	nextChar();
	std::string val;
	auto outIt = std::back_inserter(val);
	bool escaping = false;
	while(!(!escaping && curChar == delimiter)) {
		if(curChar == UNI_EOI) {
			error("Unfinished string literal");
		}
		if(escaping) {
			if(curChar == delimiter || curChar == '\\') {
				utf8::append(curChar, outIt);
			} else {
				uni_cp cp = 0;
				if(curChar == 'u' || curChar == 'U') {
					bool isBMP = curChar == 'u';
					for(int i = 1; i <= (isBMP ? 4 : 6); ++i) {
						cp <<= 4;
						if('0' <= peekChar && peekChar <= '9') {
							cp += peekChar - '0';
						} else if('a' <= peekChar && peekChar <= 'f') {
							cp += 10 + (peekChar - 'a');
						} else if('A' <= peekChar && peekChar <= 'F') {
							cp += 10 + (peekChar - 'A');
						} else {
							error("Non-hex digit found in Unicode escape sequence: " + strFromCP(peekChar));
						}
						nextChar();
					}
				} else {
					switch(curChar) {
					case 'n': cp = '\n'; break;
					case 'r': cp = '\r'; break;
					case 't': cp = '\t'; break;
					default:
						error("Unknown escape sequence in string: \\" + strFromCP(curChar));
					}
				}
				utf8::append(cp, outIt);
			}
			escaping = false;
		} else {
			if(curChar == '\\') {
				escaping = true;
			} else {
				utf8::append(curChar, outIt);
			}
		}
		nextChar();
	}
	nextChar(); // skip end delimiter
	return std::unique_ptr<Node>(new NodeString(val));
}

std::unordered_set<uni_cp> symbolChars = {
	'=', ',', '(', ')', ':',
	'+', '-', '*', '/', '^',
};

std::unordered_set<std::string> symbolStrings = {
	"=="
};

template <typename C>
std::unique_ptr<Node> Parser<C>::lexSymbol() {
	if(peekChar != UNI_EOI && symbolStrings.find(strFromCP(curChar) + strFromCP(peekChar)) != symbolStrings.end()) {
		std::string sym = strFromCP(curChar) + strFromCP(peekChar);
		nextChar(); nextChar();
		return std::unique_ptr<Node>(new NodeSymbol(sym));
	} else if(symbolChars.find(curChar) != symbolChars.end()) {
		std::string sym = strFromCP(curChar);
		nextChar();
		return std::unique_ptr<Node>(new NodeSymbol(sym));
	} else {
		error("Unexpected character: " + strFromCP(curChar));
	}
}

template <typename C>
std::unique_ptr<Node> Parser<C>::lexToken() {
	std::unique_ptr<Node> token;
	
	if(curLine == 0) {
		curLine++;
		token = lexNewline();
	} else if(curChar >= '0' && curChar <= '9')
		token = lexNumber();
	else if(isIdStart(curChar))
		token = lexId();
	else if(curChar == '\'' || curChar == '"')
		token = lexString();
	else if(curChar == '\n')
		token = lexNewline();
	else if(curChar == UNI_EOI)
		token = std::unique_ptr<Node>(new Node(NodeType::EOI));
	else
		token = lexSymbol();
	
	skipSpace();
	return token;
}

template <typename C>
std::unique_ptr<Node> Parser<C>::nextToken() {
	std::unique_ptr<Node> token = lexToken(); // Get new token
	curToken.swap(peekToken);
	peekToken.swap(token);
	return token; // Return old token
}

template <typename C>
void Parser<C>::discardToken(NodeType type) {
	std::unique_ptr<Node> token = nextToken();
	if(token->type != type)
		error("Expected " + nodeTypeDesc(type) + ", got " + token->toString());
}

template <typename C>
bool Parser<C>::isCurSymbol(std::string sym) {
	if(curToken->type != NodeType::SYM) return false;
	NodeSymbol* symToken = static_cast<NodeSymbol*>(curToken.get());
	return symToken->val == sym;
}

std::unordered_set<NodeType> terminals = { NodeType::ID, NodeType::INT, NodeType::REAL, NodeType::STR };
std::unordered_set<std::string> terminalSymbols = { "nil", "true", "false" };
std::unordered_set<std::string> prefixOperators = { "+", "-", "not" };
std::unordered_set<std::string> infixOperators = { "+", "-", "*", "/", "^", "and", "or", "(", "==" };
std::unordered_set<std::string> rightAssociativeOperators = { "^" };

std::unordered_map<std::string, int> operatorPrecedence = {
	{"and", 2}, {"or", 2},
	{"not", 4},
	{"==", 6},
	{"+", 8}, {"-", 8},
	{"*", 10}, {"/", 10},
	{"^", 12},
	{"(", 14}
};

template <typename C>
int Parser<C>::getInfixPrecedence() {
	if(curToken->type == NodeType::SYM) {
		std::string sym = static_cast<NodeSymbol*>(curToken.get())->val;
		if(infixOperators.find(sym) != infixOperators.end()) {
			return operatorPrecedence[sym];
		}
	}
	return -1; // Not an infix/postfix operator
}

template <typename C>
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
			if(isCurSymbol(")")) {
				nextToken();
			} else {
				error("Expected ')' symbol, got " + curToken->toString());
			}
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
				exp = std::unique_ptr<Node>(new NodeBinary("call", std::move(exp), parseExpr(0)));
				if(isCurSymbol(")")) {
					nextToken();
				} else {
					error("Expected ')' symbol, got " + curToken->toString());
				}
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

template <typename C>
void Parser<C>::finishStatement() {
	if(!(curToken->type == NodeType::EOI || curToken->type == NodeType::DEDENT))
		discardToken(NodeType::NL);
}

template <typename C>
std::unique_ptr<Node> Parser<C>::parseStatement() {
	if(isCurSymbol("let")) {
		nextToken();
		std::unique_ptr<Node> idToken = nextToken();
		if(idToken->type != NodeType::ID)
			error("Expected identifier after 'let', got " + nodeTypeDesc(idToken->type));
		std::string id = static_cast<NodeId*>(idToken.get())->val;
		if(!isCurSymbol("="))
			error("Expected '=' after 'let' + identifier, got " + curToken->toString());
		nextToken();
		std::unique_ptr<Node> expr = parseExpr();
		finishStatement();
		return std::unique_ptr<Node>(new NodeLet(id, std::move(expr)));
	} else if(isCurSymbol("log")) {
		nextToken();
		std::unique_ptr<Node> expr = parseExpr();
		finishStatement();
		return std::unique_ptr<Node>(new NodeLog(std::move(expr)));
	} else if(curToken->type == NodeType::ID && peekToken->type == NodeType::SYM && static_cast<NodeSymbol&>(*peekToken).val == "=") {
		std::unique_ptr<Node> idToken = nextToken();
		std::string id = static_cast<NodeId*>(idToken.get())->val;
		nextToken();
		std::unique_ptr<Node> expr = parseExpr();
		finishStatement();
		return std::unique_ptr<Node>(new NodeSet(id, std::move(expr)));
	} else if(isCurSymbol("if")) {
		nextToken();
		std::unique_ptr<Node> cond = parseExpr();
		if(!isCurSymbol(":"))
			error("Expected ':' after 'if', got " + curToken->toString());
		nextToken();
		std::unique_ptr<Node> block = parseIndentedBlock();
		return std::unique_ptr<Node>(new NodeIf(std::move(cond), std::move(block)));
	} else {
		std::unique_ptr<Node> expr = parseExpr();
		finishStatement();
		return std::unique_ptr<Node>(new NodeExprStat(std::move(expr)));
	}
}

template <typename C>
std::unique_ptr<Node> Parser<C>::parseBlock() {
	std::vector<std::unique_ptr<Node>> statements;
	while(true) {
		statements.push_back(parseStatement());
		if(curToken->type == NodeType::DEDENT || curToken->type == NodeType::EOI) break;
	}
	return std::unique_ptr<NodeBlock>(new NodeBlock(std::move(statements)));
}

template <typename C>
std::unique_ptr<Node> Parser<C>::parseIndentedBlock() {
	if(curToken->type != NodeType::INDENT)
		error("Expected indent, got " + curToken->toString());
	std::string oldIndent = static_cast<NodeIndent&>(*curToken).oldIndent;
	nextToken();
	std::unique_ptr<Node> block = parseBlock();
	if(curToken->type == NodeType::DEDENT) {
		std::string newIndent = static_cast<NodeDedent&>(*curToken).newIndent;
		if(newIndent == oldIndent) nextToken();
	}
	return block;
}

template <typename C>
std::unique_ptr<Node> Parser<C>::parseProgram() {
	discardToken(NodeType::NL);
	return parseBlock();
}


Parser<std::istreambuf_iterator<char>> newFileParser(std::ifstream& fs) {
	if(!fs.is_open()) throw ParseError("Unable to open file");
	std::istreambuf_iterator<char> it(fs);
	std::istreambuf_iterator<char> end_it;
	return Parser<std::istreambuf_iterator<char>>(it, end_it);
}

#endif