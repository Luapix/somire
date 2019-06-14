#ifndef PARSER_IMPL_HPP
#define PARSER_IMPL_HPP

ParseError::ParseError(std::string reason) : reason(reason) { }

const char* ParseError::what() const noexcept {
	return ("Parse error: " + reason + "\n").c_str();
}

template <typename C>
Parser<C>::Parser(C start, C end) : curByte(start), end(end) {
	next();
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
void Parser<C>::printAll() {
	std::ostreambuf_iterator<char> out_it(std::cout);
	while(curChar != UNI_EOI) {
		utf8::append(curChar, out_it);
		std::cout << " " << is_id_start(curChar) << " " << is_id_continue(curChar) << std::endl;
		next();
	}
}

Parser<std::istreambuf_iterator<char>> newFileParser(std::ifstream& fs) {
	if(!fs.is_open()) throw ParseError("unable to open file");
	std::istreambuf_iterator<char> it(fs);
	std::istreambuf_iterator<char> end_it;
	return Parser<std::istreambuf_iterator<char>>(it, end_it);
}

#endif