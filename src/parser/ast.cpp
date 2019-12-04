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

NodeExp::NodeExp(NodeType type) : Node(type), valueType(nullptr) {}

void NodeExp::markChildren() {
	if(valueType)
		valueType->mark();
}

NodeId::NodeId(std::string val) : NodeExp(NodeType::ID), val(val) {}

std::string NodeId::getDataDesc(std::string prefix) {
	return " " + val;
}

NodeInt::NodeInt(std::int32_t val) : NodeExp(NodeType::INT), val(val) {}

std::string NodeInt::getDataDesc(std::string prefix) {
	return " " + std::to_string(val);
}

NodeReal::NodeReal(double val) : NodeExp(NodeType::REAL), val(val) {}

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

NodeString::NodeString(std::string val) : NodeExp(NodeType::STR), val(val) {}

std::string NodeString::getDataDesc(std::string prefix) {
	return " " + escapeString(val);
}

NodeSymbol::NodeSymbol(std::string val) : NodeExp(NodeType::SYM), val(val) {}

std::string NodeSymbol::getDataDesc(std::string prefix) { return " " + val; }

NodeUnary::NodeUnary(std::string op, NodeExp* val)
	: NodeExp(NodeType::UNI_OP), op(op), val(val) {}

void NodeUnary::markChildren() {
	NodeExp::markChildren();
	val->mark();
}

std::string NodeUnary::getDataDesc(std::string prefix) { return " " + op + " " + val->toString(prefix); }

NodeBinary::NodeBinary(std::string op, NodeExp* left, NodeExp* right)
	: NodeExp(NodeType::BIN_OP), op(op), left(left), right(right) {}

void NodeBinary::markChildren() {
	NodeExp::markChildren();
	left->mark();
	right->mark();
}

std::string NodeBinary::getDataDesc(std::string prefix) { return " " + op + " " + left->toString(prefix) + " " + right->toString(prefix); }

NodeCall::NodeCall(NodeExp* func, std::vector<NodeExp*> args)
	: NodeExp(NodeType::CALL), func(func), args(args) {}

void NodeCall::markChildren() {
	NodeExp::markChildren();
	func->mark();
	for(Node* arg : args) {
		arg->mark();
	}
}

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

NodeLet::NodeLet(std::string id, NodeExp* exp)
	: Node(NodeType::LET), id(id), exp(exp) {}

void NodeLet::markChildren() { exp->mark(); }

std::string NodeLet::getDataDesc(std::string prefix) { return " " + id + " = " + exp->toString(prefix); }

NodeSet::NodeSet(std::string id, NodeExp* exp)
	: Node(NodeType::SET), id(id), exp(exp) {}

void NodeSet::markChildren() { exp->mark(); }

std::string NodeSet::getDataDesc(std::string prefix) { return " " + id + " = " + exp->toString(prefix); }

NodeExprStat::NodeExprStat(NodeExp* exp) : Node(NodeType::EXPR_STAT), exp(exp) {}

void NodeExprStat::markChildren() { exp->mark(); }

std::string NodeExprStat::getDataDesc(std::string prefix) { return " " + exp->toString(prefix); }

NodeIf::NodeIf(NodeExp* cond, Node* thenBlock) : Node(NodeType::IF),
	cond(cond), thenBlock(thenBlock), elseBlock(nullptr) {}

void NodeIf::markChildren() {
	cond->mark();
	thenBlock->mark();
	if(elseBlock)
		elseBlock->mark();
}

std::string NodeIf::getDataDesc(std::string prefix) {
	if(elseBlock)
		return " " + cond->toString(prefix) + ": " + thenBlock->toString(prefix) + " else: " + elseBlock->toString(prefix);
	else
		return " " + cond->toString(prefix) + ": " + thenBlock->toString(prefix);
}

NodeWhile::NodeWhile(NodeExp* cond, Node* block) : Node(NodeType::WHILE),
	cond(cond), block(block) {}

void NodeWhile::markChildren() { cond->mark(); block->mark(); }

std::string NodeWhile::getDataDesc(std::string prefix) { return " " + cond->toString(prefix) + ": " + block->toString(prefix); }

NodeReturn::NodeReturn(NodeExp* exp) : Node(NodeType::RETURN), exp(exp) {}

void NodeReturn::markChildren() { exp->mark(); }

std::string NodeReturn::getDataDesc(std::string prefix) { return " " + exp->toString(prefix); }

NodeFunction::NodeFunction(std::vector<std::string> argNames, std::vector<Node*> argTypes, Node* block)
	: NodeExp(NodeType::FUNC), argNames(argNames), argTypes(argTypes), block(block), protoIdx(-1) {}

void NodeFunction::markChildren() {
	NodeExp::markChildren();
	for(Node* arg : argTypes) {
		arg->mark();
	}
	block->mark();
}

std::string NodeFunction::getDataDesc(std::string prefix) {
	std::string res = "(";
	for(uint32_t i = 0; i < argNames.size(); i++) {
		res += argNames[i];
		if(i != argNames.size()-1)
			res += ", ";
	}
	return res + "): " + block->toString(prefix);
}

NodeBlock::NodeBlock(std::vector<Node*> statements)
	: Node(NodeType::BLOCK), statements(statements) {}

void NodeBlock::markChildren() {
	for(Node* stat : statements) {
		stat->mark();
	}
}

std::string NodeBlock::getDataDesc(std::string prefix) {
	std::string desc;
	std::string newPrefix = prefix + "  ";
	for(auto& stat : statements) {
		desc += newPrefix + stat->toString(newPrefix) + "\n";
	}
	return ":\n" + desc + prefix;
}

NodeList::NodeList(std::vector<NodeExp*> vals)
	: NodeExp(NodeType::LIST), vals(vals) {}

void NodeList::markChildren() {
	NodeExp::markChildren();
	for(Node* val : vals) {
		val->mark();
	}
}

std::string NodeList::getDataDesc(std::string prefix) {
	std::string res = " [";
	for(uint32_t i = 0; i < vals.size(); i++) {
		res += vals[i]->toString();
		if(i != vals.size()-1)
			res += ", ";
	}
	return res + "]";
}

NodeProp::NodeProp(NodeExp* val, std::string prop)
	: NodeExp(NodeType::PROP), val(val), prop(prop) {}

void NodeProp::markChildren() {
	NodeExp::markChildren();
	val->mark();
}

std::string NodeProp::getDataDesc(std::string prefix) { return " " + prefix + " of " + val->toString(prefix); }

NodeSimpleType::NodeSimpleType(std::string name)
	: Node(NodeType::SIMPLE_TYPE), name(name) {}

std::string NodeSimpleType::getDataDesc(std::string prefix) {
	return " '" + name + "'";
}
