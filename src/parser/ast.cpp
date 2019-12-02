#include "ast.hpp"

#include "util/util.hpp"

ParseError::ParseError(const std::string& what)
	: runtime_error(what) { }

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
	case NodeType::CALL: return "call";
	case NodeType::LET: return "let";
	case NodeType::SET: return "set";
	case NodeType::EXPR_STAT: return "expression statement";
	case NodeType::IF: return "if";
	case NodeType::WHILE: return "while";
	case NodeType::RETURN: return "return";
	case NodeType::FUNC: return "function";
	case NodeType::BLOCK: return "block";
	case NodeType::LIST: return "list";
	case NodeType::PROP: return "prop";
	case NodeType::SIMPLE_TYPE: return "simple type";
	default:
		throw std::runtime_error("Unknown node type");
	}
}

Node::Node(NodeType type) : type(type) { }

std::string Node::toString(std::string prefix) {
	return "<" + nodeTypeDesc(type) + getDataDesc(prefix) + ">";
}

std::string Node::getDataDesc(std::string prefix) { return ""; }

NodeIndent::NodeIndent(std::string oldIndent) : Node(NodeType::INDENT), oldIndent(oldIndent) {}
NodeDedent::NodeDedent(std::string newIndent) : Node(NodeType::DEDENT), newIndent(newIndent) {}

NodeId::NodeId(std::string val) : Node(NodeType::ID), val(val) {}

std::string NodeId::getDataDesc(std::string prefix) {
	return " " + val;
}

NodeInt::NodeInt(std::int32_t val) : Node(NodeType::INT), val(val) {}

std::string NodeInt::getDataDesc(std::string prefix) {
	return " " + std::to_string(val);
}

NodeReal::NodeReal(double val) : Node(NodeType::REAL), val(val) {}

std::string NodeReal::getDataDesc(std::string prefix) {
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

std::string NodeString::getDataDesc(std::string prefix) {
	return " " + escapeString(val);
}

NodeSymbol::NodeSymbol(std::string val) : Node(NodeType::SYM), val(val) {}

std::string NodeSymbol::getDataDesc(std::string prefix) { return " " + val; }

NodeUnary::NodeUnary(std::string op, std::unique_ptr<Node> val)
	: Node(NodeType::UNI_OP), op(op), val(std::move(val)) {}

std::string NodeUnary::getDataDesc(std::string prefix) { return " " + op + " " + val->toString(prefix); }

NodeBinary::NodeBinary(std::string op, std::unique_ptr<Node> left, std::unique_ptr<Node> right)
	: Node(NodeType::BIN_OP), op(op), left(std::move(left)), right(std::move(right)) {}

std::string NodeBinary::getDataDesc(std::string prefix) { return " " + op + " " + left->toString(prefix) + " " + right->toString(prefix); }

NodeCall::NodeCall(std::unique_ptr<Node> func, std::vector<std::unique_ptr<Node>> args)
	: Node(NodeType::CALL), func(std::move(func)), args(std::move(args)) {}

std::string NodeCall::getDataDesc(std::string prefix) {
	std::string res = " " + func->toString(prefix) + " (";
	for(uint32_t i = 0; i < args.size(); i++) {
		res += args[i]->toString();
		if(i != args.size() - 1)
			res += ", ";
	}
	res += ")";
	return res;
}

NodeLet::NodeLet(std::string id, std::unique_ptr<Node> exp)
	: Node(NodeType::LET), id(id), exp(std::move(exp)) {}

std::string NodeLet::getDataDesc(std::string prefix) { return " " + id + " = " + exp->toString(prefix); }

NodeSet::NodeSet(std::string id, std::unique_ptr<Node> exp)
	: Node(NodeType::SET), id(id), exp(std::move(exp)) {}

std::string NodeSet::getDataDesc(std::string prefix) { return " " + id + " = " + exp->toString(prefix); }

NodeExprStat::NodeExprStat(std::unique_ptr<Node> exp) : Node(NodeType::EXPR_STAT), exp(std::move(exp)) {}

std::string NodeExprStat::getDataDesc(std::string prefix) { return " " + exp->toString(prefix); }

NodeIf::NodeIf(std::unique_ptr<Node> cond, std::unique_ptr<Node> thenBlock) : Node(NodeType::IF),
	cond(std::move(cond)), thenBlock(std::move(thenBlock)), elseBlock(nullptr) {}

std::string NodeIf::getDataDesc(std::string prefix) {
	if(elseBlock)
		return " " + cond->toString(prefix) + ": " + thenBlock->toString(prefix) + " else: " + elseBlock->toString(prefix);
	else
		return " " + cond->toString(prefix) + ": " + thenBlock->toString(prefix);
}

NodeWhile::NodeWhile(std::unique_ptr<Node> cond, std::unique_ptr<Node> block) : Node(NodeType::WHILE),
	cond(std::move(cond)), block(std::move(block)) {}

std::string NodeWhile::getDataDesc(std::string prefix) { return " " + cond->toString(prefix) + ": " + block->toString(prefix); }

NodeReturn::NodeReturn(std::unique_ptr<Node> expr) : Node(NodeType::RETURN), expr(std::move(expr)) {}

std::string NodeReturn::getDataDesc(std::string prefix) { return " " + expr->toString(prefix); }

NodeFunction::NodeFunction(std::vector<std::string> argNames, std::vector<std::unique_ptr<Node>> argTypes, std::unique_ptr<Node> block)
	: Node(NodeType::FUNC), argNames(argNames), argTypes(std::move(argTypes)), block(std::move(block)) {}

std::string NodeFunction::getDataDesc(std::string prefix) {
	std::string res = "(";
	for(uint32_t i = 0; i < argNames.size(); i++) {
		res += argNames[i];
		if(i != argNames.size()-1)
			res += ", ";
	}
	return res + "): " + block->toString(prefix);
}

NodeBlock::NodeBlock(std::vector<std::unique_ptr<Node>> statements)
	: Node(NodeType::BLOCK), statements(std::move(statements)) {}

std::string NodeBlock::getDataDesc(std::string prefix) {
	std::string desc;
	std::string newPrefix = prefix + "  ";
	for(auto& stat : statements) {
		desc += newPrefix + stat->toString(newPrefix) + "\n";
	}
	return ":\n" + desc + prefix;
}

NodeList::NodeList(std::vector<std::unique_ptr<Node>> val)
	: Node(NodeType::LIST), val(std::move(val)) {}

std::string NodeList::getDataDesc(std::string prefix) {
	std::string res = " [";
	for(uint32_t i = 0; i < val.size(); i++) {
		res += val[i]->toString();
		if(i != val.size()-1)
			res += ", ";
	}
	return res + "]";
}

NodeProp::NodeProp(std::unique_ptr<Node> val, std::string prop)
	: Node(NodeType::PROP), val(std::move(val)), prop(prop) {}

std::string NodeProp::getDataDesc(std::string prefix) { return " " + prefix + " of " + val->toString(prefix); }

NodeSimpleType::NodeSimpleType(std::string name)
	: Node(NodeType::SIMPLE_TYPE), name(name) {}

std::string NodeSimpleType::getDataDesc(std::string prefix) {
	return " '" + name + "'";
}
