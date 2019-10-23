#ifndef PARSER_IMPL_HPP
#define PARSER_IMPL_HPP


template <typename C>
Parser<C>::Parser(C start, C end)
	: curByte(start), end(end), curLine(1), curIndent() {
	next(); next(); next();
}


template <typename C>
std::unique_ptr<Node> Parser<C>::lexProgram() {
	lexNewline();
	// TODO
	return lexExpr();
}

template <typename C>
[[noreturn]] void Parser<C>::error(std::string cause) {
	throw ParseError("At line " + std::to_string(curLine) + ": " + cause);
}

template <typename C>
void Parser<C>::next() {
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
	while(is_space(curChar)) {
		if(curChar == '\n') {
			if(!allowNL) return;
			lexNewline();
			return;
		} else {
			next();
		}
	}
}

template <typename C>
std::unique_ptr<Node> Parser<C>::lexNewline() {
	std::string newIndent;
	while(is_space(curChar)) {
		if(curChar == '\n') {
			curLine++;
			newIndent.clear();
		} else {
			appendCP(newIndent, curChar);
		}
		next();
	}
	NodeType type;
	if(newIndent == curIndent) {
		type = N_NL;
	} else if(newIndent.substr(0, curIndent.length()) == curIndent) {
		type = N_INDENT;
	} else if(curIndent.substr(0, newIndent.length()) == newIndent) {
		type = N_DEDENT;
	} else {
		error("Invalid indentation");
	}
	curIndent = newIndent;
	return std::unique_ptr<Node>(new Node(type));
}

template <typename C>
std::unique_ptr<Node> Parser<C>::lexId() {
	std::string val = strFromCP(curChar);
	next();
	while(is_id_continue(curChar)) {
		appendCP(val, curChar);
		next();
	}
	return std::unique_ptr<Node>(new NodeId(val));
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
				next();
				return std::unique_ptr<Node>(new NodeInt(0));
			}
			next(); next(); // skip base specifier
		}
	}
	while(isDigit(curChar, base)) {
		appendCP(s, curChar); next();
		if(curChar == '.' && !isReal && s.size() > 0 && base == 10 && isDigit(peekChar, base)) {
			isReal = true;
			appendCP(s, curChar);
			next();
		} else if((curChar == 'e' || curChar == 'E') && (isReal || (!isReal && base == 10))) {
			bool hasSign = (peekChar == '+' || peekChar == '-') && isDigit(peekChar2, 10);
			bool isExponent = isDigit(peekChar, 10) || hasSign;
			if(isExponent) {
				isReal = true;
				appendCP(s, curChar); next();
				if(hasSign) { appendCP(s, curChar); next(); }
				while(isDigit(curChar, 10)) {
					appendCP(s, curChar); next();
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
std::unique_ptr<Node> Parser<C>::lexExpr() {
	if(curChar >= '0' && curChar <= '9')
		return lexNumber();
	else if(is_id_start(curChar))
		return lexId();
	else
		error("Invalid syntax in expression");
}


Parser<std::istreambuf_iterator<char>> newFileParser(std::ifstream& fs) {
	if(!fs.is_open()) throw ParseError("Unable to open file");
	std::istreambuf_iterator<char> it(fs);
	std::istreambuf_iterator<char> end_it;
	return Parser<std::istreambuf_iterator<char>>(it, end_it);
}

#endif