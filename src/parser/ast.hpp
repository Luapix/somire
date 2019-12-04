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

class Node : public GC::GCObject {
public:
	NodeType type;
	
	Node(NodeType type);
	virtual ~Node() = default;
	
	virtual std::string toString(std::string prefix = "");
	
protected:
	virtual std::string getDataDesc(std::string prefix);
};

class NodeIndent : public Node {
public:
	NodeIndent(std::string oldIndent);
	
	std::string oldIndent;
};

class NodeDedent : public Node {
public:
	NodeDedent(std::string newIndent);
	
	std::string newIndent;
};

class NodeExp : public Node {
public:
	NodeExp(NodeType type);
	
	Type* valueType;
	
	void markChildren() override;
};

class NodeId : public NodeExp {
public:
	NodeId(std::string val);
	
	std::string val;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeInt : public NodeExp {
public:
	NodeInt(std::int32_t val);
	
	std::int32_t val;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeReal : public NodeExp {
public:
	NodeReal(double val);
	
	double val;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeString : public NodeExp {
public:
	NodeString(std::string val);
	
	std::string val;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeSymbol : public NodeExp {
public:
	NodeSymbol(std::string val);
	
	std::string val;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeUnary : public NodeExp {
public:
	NodeUnary(std::string op, NodeExp* val);
	
	std::string op;
	NodeExp* val;
	
	void markChildren() override;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeBinary : public NodeExp {
public:
	NodeBinary(std::string op, NodeExp* left, NodeExp* right);
	
	std::string op;
	NodeExp *left, *right;
	
	void markChildren() override;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeCall : public NodeExp {
public:
	NodeCall(NodeExp* func, std::vector<NodeExp*> args);
	
	NodeExp* func;
	std::vector<NodeExp*> args;
	
	void markChildren() override;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeLet : public Node {
public:
	NodeLet(std::string id, NodeExp* exp);
	
	std::string id;
	NodeExp* exp;
	
	void markChildren() override;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeSet : public Node {
public:
	NodeSet(std::string id, NodeExp* exp);
	
	std::string id;
	NodeExp* exp;
	
	void markChildren() override;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeExprStat : public Node {
public:
	NodeExprStat(NodeExp* exp);
	
	NodeExp* exp;
	
	void markChildren() override;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeIf : public Node {
public:
	NodeIf(NodeExp* cond, Node* thenBlock);
	
	NodeExp* cond;
	Node *thenBlock, *elseBlock;
	
	void markChildren() override;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeWhile : public Node {
public:
	NodeWhile(NodeExp* cond, Node* block);
	
	NodeExp* cond;
	Node* block;
	
	void markChildren() override;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeReturn : public Node {
public:
	NodeReturn(NodeExp* exp);
	
	NodeExp* exp;
	
	void markChildren() override;
	
protected:
	std::string getDataDesc(std::string prefix) override;	
};

class NodeFunction : public NodeExp {
public:
	NodeFunction(std::vector<std::string> argNames, std::vector<Node*> argTypes, Node* block);
	
	std::vector<std::string> argNames;
	std::vector<Node*> argTypes;
	Node* block;
	
	int32_t protoIdx;
	std::vector<int16_t> upvalues;
	
	void markChildren() override;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeBlock : public Node {
public:
	NodeBlock(std::vector<Node*> statements);
	
	std::vector<Node*> statements;
	
	void markChildren() override;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeList : public NodeExp {
public:
	NodeList(std::vector<NodeExp*> vals);
	
	std::vector<NodeExp*> vals;
	
	void markChildren() override;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeProp : public NodeExp {
public:
	NodeProp(NodeExp* val, std::string prop);
	
	NodeExp* val;
	std::string prop;
	
	void markChildren() override;

protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeSimpleType : public Node {
public:
	NodeSimpleType(std::string name);
	
	std::string name;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};
