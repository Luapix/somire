#include "ast.hpp"

ParseError::ParseError(const std::string& what)
	: runtime_error(what) { }


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
