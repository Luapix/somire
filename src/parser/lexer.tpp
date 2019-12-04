#pragma once

#include <climits>
#include <cstdint>
#include <stdexcept>
#include <cassert>
#include <unordered_set>

#include "util/uni_util.hpp"

template<typename C>
Lexer<C>::Lexer(C start, C end)
	: curByte(start), end(end), curLine(0), curIndent() {
	nextChar(); nextChar(); nextChar();
}

template<typename C>
[[noreturn]] void Lexer<C>::error(std::string cause, bool parseError) {
	std::string mes = "At line " + std::to_string(curLine) + ": " + cause;
	if(parseError) {
		throw ParseError("Parse error: " + mes);
	} else {
		throw ParseError("Lex error: " + mes);
	}
}

template<typename C>
void Lexer<C>::nextChar() {
	curChar = peekChar;
	peekChar = peekChar2;
	if(curByte == end) {
		peekChar2 = UNI_EOI;
	} else {
		peekChar2 = utf8::next(curByte, end);
	}
}

template<typename C>
void Lexer<C>::skipSpace(bool allowNL) {
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

template<typename C>
GC::Root<Node> Lexer<C>::lexNewline() {
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
	GC::Root<Node> newline;
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
	"let", "if", "else", "while",
	"not", "and", "or",
	"nil", "true", "false",
	"return",
	"fun"
};

template<typename C>
GC::Root<Node> Lexer<C>::lexId() {
	std::string val = strFromCP(curChar);
	nextChar();
	while(isIdContinue(curChar)) {
		appendCP(val, curChar);
		nextChar();
	}
	if(keywords.find(val) != keywords.end()) {
		return GC::Root<Node>(new NodeSymbol(val));
	} else {
		return GC::Root<Node>(new NodeId(val));
	}
}

#if INT_MAX != INT32_MAX
	#error Can''t be bothered to deal with 16-bit ints
#endif
template<typename C>
GC::Root<Node> Lexer<C>::lexNumber() {
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
				return GC::Root<Node>(new NodeInt(0));
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
		return GC::Root<Node>(new NodeReal(val));
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
		return GC::Root<Node>(new NodeInt(val2));
	}
}

template<typename C>
GC::Root<Node> Lexer<C>::lexString() {
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
	return GC::Root<Node>(new NodeString(val));
}

std::unordered_set<uni_cp> symbolChars = {
	'=', ',', '(', ')', ':',
	'+', '-', '*', '/', '^', '%',
	'<', '>',
	'[', ']',
	'.'
};

std::unordered_set<std::string> symbolStrings = {
	"==", "!=", "<=", ">="
};

template<typename C>
GC::Root<Node> Lexer<C>::lexSymbol() {
	if(peekChar != UNI_EOI && symbolStrings.find(strFromCP(curChar) + strFromCP(peekChar)) != symbolStrings.end()) {
		std::string sym = strFromCP(curChar) + strFromCP(peekChar);
		nextChar(); nextChar();
		return GC::Root<Node>(new NodeSymbol(sym));
	} else if(symbolChars.find(curChar) != symbolChars.end()) {
		std::string sym = strFromCP(curChar);
		nextChar();
		return GC::Root<Node>(new NodeSymbol(sym));
	} else {
		error("Unexpected character: " + strFromCP(curChar));
	}
}

template<typename C>
GC::Root<Node> Lexer<C>::lexToken() {
	GC::Root<Node> token;
	
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
		token = GC::Root<Node>(new Node(NodeType::EOI));
	else
		token = lexSymbol();
	
	skipSpace();
	return token;
}
