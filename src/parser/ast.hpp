#pragma once

#include <stdexcept>
#include <string>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <memory>
#include <vector>

#include "util/gc.hpp"
#include "compiler/types.hpp"

class ParseError : public std::runtime_error {
public:
	ParseError(const std::string& what);
};


enum class NodeType {
	NL, INDENT, DEDENT, EOI,
	ID, INT, REAL, STR,
	SYM,
	UNI_OP, BIN_OP, CALL,
	LET, SET, EXPR_STAT, IF, WHILE, RETURN,
	FUNC,
	BLOCK,
	LIST,
	PROP,
	SIMPLE_TYPE
};

std::string nodeTypeDesc(NodeType type);

class Node {
public:
	const NodeType type;
	
	Node(NodeType type);
	virtual ~Node() = default;
	
	virtual std::string toString(std::string prefix = "");
	
protected:
	virtual std::string getDataDesc(std::string prefix);
};

class NodeIndent : public Node {
public:
	NodeIndent(std::string oldIndent);
	
	const std::string oldIndent;
};

class NodeDedent : public Node {
public:
	NodeDedent(std::string newIndent);
	
	const std::string newIndent;
};

class NodeExp : public Node {
public:
	using Node::Node;
	
	GC::Root<Type> valueType;
};

class NodeId : public NodeExp {
public:
	NodeId(std::string val);
	
	const std::string val;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeInt : public NodeExp {
public:
	NodeInt(std::int32_t val);
	
	const std::int32_t val;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeReal : public NodeExp {
public:
	NodeReal(double val);
	
	const double val;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeString : public NodeExp {
public:
	NodeString(std::string val);
	
	const std::string val;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeSymbol : public NodeExp {
public:
	NodeSymbol(std::string val);
	
	const std::string val;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeUnary : public NodeExp {
public:
	NodeUnary(std::string op, std::unique_ptr<NodeExp> val);
	
	const std::string op;
	const std::unique_ptr<NodeExp> val;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeBinary : public NodeExp {
public:
	NodeBinary(std::string op, std::unique_ptr<NodeExp> left, std::unique_ptr<NodeExp> right);
	
	const std::string op;
	const std::unique_ptr<NodeExp> left, right;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeCall : public NodeExp {
public:
	NodeCall(std::unique_ptr<NodeExp> func, std::vector<std::unique_ptr<NodeExp>> args);
	
	const std::unique_ptr<NodeExp> func;
	const std::vector<std::unique_ptr<NodeExp>> args;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeLet : public Node {
public:
	NodeLet(std::string id, std::unique_ptr<NodeExp> exp);
	
	const std::string id;
	const std::unique_ptr<NodeExp> exp;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeSet : public Node {
public:
	NodeSet(std::string id, std::unique_ptr<NodeExp> exp);
	
	const std::string id;
	const std::unique_ptr<NodeExp> exp;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeExprStat : public Node {
public:
	NodeExprStat(std::unique_ptr<NodeExp> exp);
	
	const std::unique_ptr<NodeExp> exp;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeIf : public Node {
public:
	NodeIf(std::unique_ptr<NodeExp> cond, std::unique_ptr<Node> thenBlock);
	
	const std::unique_ptr<NodeExp> cond;
	const std::unique_ptr<Node> thenBlock;
	std::unique_ptr<Node> elseBlock;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeWhile : public Node {
public:
	NodeWhile(std::unique_ptr<NodeExp> cond, std::unique_ptr<Node> block);
	
	const std::unique_ptr<NodeExp> cond;
	const std::unique_ptr<Node> block;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeReturn : public Node {
public:
	NodeReturn(std::unique_ptr<NodeExp> expr);
	
	const std::unique_ptr<NodeExp> expr;
	
protected:
	std::string getDataDesc(std::string prefix) override;	
};

class NodeFunction : public NodeExp {
public:
	NodeFunction(std::vector<std::string> argNames, std::vector<std::unique_ptr<Node>> argTypes, std::unique_ptr<Node> block);
	
	const std::vector<std::string> argNames;
	const std::vector<std::unique_ptr<Node>> argTypes;
	const std::unique_ptr<Node> block;
	
	int32_t protoIdx;
	std::vector<int16_t> upvalues;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeBlock : public Node {
public:
	NodeBlock(std::vector<std::unique_ptr<Node>> statements);
	
	const std::vector<std::unique_ptr<Node>> statements;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeList : public NodeExp {
public:
	NodeList(std::vector<std::unique_ptr<NodeExp>> val);
	
	const std::vector<std::unique_ptr<NodeExp>> val;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeProp : public NodeExp {
public:
	NodeProp(std::unique_ptr<NodeExp> val, std::string prop);
	
	const std::unique_ptr<NodeExp> val;
	const std::string prop;

protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeSimpleType : public Node {
public:
	NodeSimpleType(std::string name);
	
	const std::string name;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};
