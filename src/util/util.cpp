#include "util.hpp"

#include <iomanip>
#include <iterator>
#include <sstream>

#include "utf8.h"
#include "uni_util.hpp"

std::string escapeString(std::string s) {
	std::string res;
	auto outIt = std::back_inserter(res);
	auto inIt = s.begin();
	auto inEnd = s.end();
	while(inIt != inEnd) {
		uni_cp cp = utf8::next(inIt, inEnd);
		switch(cp) {
		case '\n': res += "\\n"; break;
		case '\r': res += "\\r"; break;
		case '\t': res += "\\t"; break;
		case '\\':
		case '\'':
			res += "\\" + strFromCP(cp);
			break;
		default:
			if(isGraphic(cp)) {
				utf8::append(cp, outIt);
			} else {
				std::stringstream code;
				code << "\\" << (cp <= 0xffff ? "u" : "U")
					<< std::setfill('0') << std::setw(cp <= 0xffff ? 4 : 6) << std::hex << cp;
				res += code.str();
			}
		}
	}
	return "'" + res + "'";
}
