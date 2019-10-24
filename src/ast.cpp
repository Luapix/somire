#include "ast.hpp"

#include "utf8.h"

ParseError::ParseError(const std::string& what)
	: runtime_error("Parse error: " + what) { }


Node::Node(NodeType type) : type(type) { }

std::string Node::toString() {
	switch(type) {
	case N_NL:
		return "<NL>";
	case N_INDENT:
		return "<INDENT>";
	case N_DEDENT:
		return "<DEDENT>";
	default:
		throw std::runtime_error("Unimplemented token type");
	}
}

NodeId::NodeId(std::string val) : Node(N_ID), val(val) {}

std::string NodeId::toString() {
	return "<ID " + val + ">";
}

NodeInt::NodeInt(std::int32_t val) : Node(N_INT), val(val) {}

std::string NodeInt::toString() {
	return "<INT " + std::to_string(val) + ">";
}

NodeReal::NodeReal(double val) : Node(N_REAL), val(val) {}

std::string NodeReal::toString() {
	char buf[50];
	int res = std::snprintf(buf, 48, "%.16g", val);
	assert(res >= 0 && res < 48);
	bool hasPeriod = false;
	for(auto i = 0; i < res; i++) {
		if(buf[i] == '.') {
			hasPeriod = true;
			break;
		}
	}
	if(!hasPeriod) {
		buf[res] = '.';
		buf[res+1] = '0';
		buf[res+2] = '\0';
	}
	return "<REAL " + std::string(buf) + ">";
}

NodeString::NodeString(std::string val) : Node(N_STR), val(val) {}

std::string NodeString::toString() {
	std::string res;
	auto outIt = std::back_inserter(res);
	auto inIt = val.begin();
	auto inEnd = val.end();
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
	return "<STR '" + res + "'>";
}
