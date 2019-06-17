#include "uni_util.hpp"

void appendCP(std::string& s, uni_cp cp) {
	utf8::append(cp, std::back_inserter(s));
}

std::string strFromCP(uni_cp cp) {
	std::string s;
	appendCP(s, cp);
	return s;
}

bool isDigit(uni_cp cp) {
	return '0' <= cp && cp <= '9';
}
