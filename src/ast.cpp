#include "ast.hpp"

#include "util.hpp"

ParseError::ParseError(const std::string& what)
	: runtime_error("Parse error: " + what) { }

std::string nodeTypeDesc(NodeType type) {
	switch(type) {
	case NodeType::NL: return "newline";
	case NodeType::INDENT: return "indent";
	case NodeType::DEDENT: return "dedent";
	case NodeType::EOI: return "EOI";
	case NodeType::ID: return "identifier";
	case NodeType::INT: return "int";
	case NodeType::REAL: return "real";
	case NodeType::STR: return "string";
	case NodeType::SYM: return "symbol";
	case NodeType::UNI_OP: return "unitary";
	case NodeType::BIN_OP: return "binary";
	case NodeType::LET: return "let";
	case NodeType::SET: return "set";
	case NodeType::EXPR_STAT: return "expression statement";
	case NodeType::LOG: return "log";
	case NodeType::BLOCK: return "block";
	default:
		throw std::runtime_error("Unknown node type");
	}
}

Node::Node(NodeType type) : type(type) { }

std::string Node::toString() {
	return "<" + nodeTypeDesc(type) + getDataDesc() + ">";
}

std::string Node::getDataDesc() { return ""; }

NodeId::NodeId(std::string val) : Node(NodeType::ID), val(val) {}

std::string NodeId::getDataDesc() {
	return " " + val;
}

NodeInt::NodeInt(std::int32_t val) : Node(NodeType::INT), val(val) {}

std::string NodeInt::getDataDesc() {
	return " " + std::to_string(val);
}

NodeReal::NodeReal(double val) : Node(NodeType::REAL), val(val) {}

std::string NodeReal::getDataDesc() {
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
	return " " + std::string(buf);
}

NodeString::NodeString(std::string val) : Node(NodeType::STR), val(val) {}

std::string NodeString::getDataDesc() {
	return " " + escapeString(val);
}

NodeSymbol::NodeSymbol(std::string val) : Node(NodeType::SYM), val(val) {}

std::string NodeSymbol::getDataDesc() { return " " + val; }

NodeUnary::NodeUnary(std::string op, std::unique_ptr<Node> val)
	: Node(NodeType::UNI_OP), op(op), val(std::move(val)) {}

std::string NodeUnary::getDataDesc() { return " " + op + " " + val->toString(); }

NodeBinary::NodeBinary(std::string op, std::unique_ptr<Node> left, std::unique_ptr<Node> right)
	: Node(NodeType::BIN_OP), op(op), left(std::move(left)), right(std::move(right)) {}

std::string NodeBinary::getDataDesc() { return " " + op + " " + left->toString() + " " + right->toString(); }

NodeLet::NodeLet(std::string id, std::unique_ptr<Node> exp)
	: Node(NodeType::LET), id(id), exp(std::move(exp)) {}

std::string NodeLet::getDataDesc() { return " " + id + " = " + exp->toString(); }

NodeSet::NodeSet(std::string id, std::unique_ptr<Node> exp)
	: Node(NodeType::SET), id(id), exp(std::move(exp)) {}

std::string NodeSet::getDataDesc() { return " " + id + " = " + exp->toString(); }

NodeExprStat::NodeExprStat(std::unique_ptr<Node> exp) : Node(NodeType::EXPR_STAT), exp(std::move(exp)) {}

std::string NodeExprStat::getDataDesc() { return " " + exp->toString(); }

NodeLog::NodeLog(std::unique_ptr<Node> exp) : Node(NodeType::LOG), exp(std::move(exp)) {}

std::string NodeLog::getDataDesc() { return " " + exp->toString(); }

NodeBlock::NodeBlock(std::vector<std::unique_ptr<Node>> statements)
	: Node(NodeType::BLOCK), statements(std::move(statements)) {}

std::string NodeBlock::getDataDesc() {
	std::string desc;
	for(auto& stat : statements) {
		desc += "  " + stat->toString() + "\n";
	}
	return ":\n" + desc;
}
