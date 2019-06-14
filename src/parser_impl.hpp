#ifndef PARSER_IMPL_HPP
#define PARSER_IMPL_HPP


template <typename C>
Parser<C>::Parser(C start, C end)
	: curByte(start), end(end), curLine(1), curIndent() {
	next();
}


template <typename C>
std::unique_ptr<Node> Parser<C>::lexProgram() {
	lexNewline();
	// TODO
	return lexId();
}

template <typename C>
void Parser<C>::error(std::string cause) {
	throw ParseError("At line " + std::to_string(curLine) + ": " + cause);
}

template <typename C>
void Parser<C>::next() {
	if(curByte == end) {
		curChar = UNI_EOI;
	} else {
		curChar = utf8::next(curByte, end);
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


Parser<std::istreambuf_iterator<char>> newFileParser(std::ifstream& fs) {
	if(!fs.is_open()) throw ParseError("unable to open file");
	std::istreambuf_iterator<char> it(fs);
	std::istreambuf_iterator<char> end_it;
	return Parser<std::istreambuf_iterator<char>>(it, end_it);
}

#endif