#include "ast.hpp"

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