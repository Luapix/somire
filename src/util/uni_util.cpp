#include "uni_util.hpp"

void appendCP(std::string& s, uni_cp cp) {
	utf8::append(cp, std::back_inserter(s));
}

std::string strFromCP(uni_cp cp) {
	std::string s;
	appendCP(s, cp);
	return s;
}

bool isDigit(uni_cp cp, unsigned int base) {
	if(base <= 10) {
		return '0' <= cp && cp < '0' + base;
	} else if(base == 16) {
		return ('0' <= cp && cp <= '9')
			|| ('a' <= cp && cp <= 'f')
			|| ('A' <= cp && cp <= 'F');
	} else {
		return false;
	}
}
