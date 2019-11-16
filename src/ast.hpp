#pragma once

#include <stdexcept>
#include <string>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <memory>
#include <vector>

class ParseError : public std::runtime_error {
public:
	ParseError(const std::string& what);
};


enum class NodeType {
	NL, INDENT, DEDENT, EOI,
	ID, INT, REAL, STR,
	SYM,
	UNI_OP, BIN_OP, CALL,
	LET, SET, EXPR_STAT, IF, WHILE,
	FUNC,
	BLOCK
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

class NodeId : public Node {
public:
	NodeId(std::string val);
	
	const std::string val;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeInt : public Node {
public:
	NodeInt(std::int32_t val);
	
	const std::int32_t val;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeReal : public Node {
public:
	NodeReal(double val);
	
	const double val;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeString : public Node {
public:
	NodeString(std::string val);
	
	const std::string val;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeSymbol : public Node {
public:
	NodeSymbol(std::string val);
	
	const std::string val;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeUnary : public Node {
public:
	NodeUnary(std::string op, std::unique_ptr<Node> val);
	
	const std::string op;
	const std::unique_ptr<Node> val;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeBinary : public Node {
public:
	NodeBinary(std::string op, std::unique_ptr<Node> left, std::unique_ptr<Node> right);
	
	const std::string op;
	const std::unique_ptr<Node> left, right;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeCall : public Node {
public:
	NodeCall(std::unique_ptr<Node> func, std::vector<std::unique_ptr<Node>> args);
	
	const std::unique_ptr<Node> func;
	const std::vector<std::unique_ptr<Node>> args;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeLet : public Node {
public:
	NodeLet(std::string id, std::unique_ptr<Node> exp);
	
	const std::string id;
	const std::unique_ptr<Node> exp;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeSet : public Node {
public:
	NodeSet(std::string id, std::unique_ptr<Node> exp);
	
	const std::string id;
	const std::unique_ptr<Node> exp;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeExprStat : public Node {
public:
	NodeExprStat(std::unique_ptr<Node> exp);
	
	const std::unique_ptr<Node> exp;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeIf : public Node {
public:
	NodeIf(std::unique_ptr<Node> cond, std::unique_ptr<Node> thenBlock);
	
	const std::unique_ptr<Node> cond;
	const std::unique_ptr<Node> thenBlock;
	std::unique_ptr<Node> elseBlock;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeWhile : public Node {
public:
	NodeWhile(std::unique_ptr<Node> cond, std::unique_ptr<Node> block);
	
	const std::unique_ptr<Node> cond;
	const std::unique_ptr<Node> block;
	
protected:
	std::string getDataDesc(std::string prefix) override;
};

class NodeFunction : public Node {
public:
	NodeFunction(std::vector<std::string> argNames, std::unique_ptr<Node> block);
	
	const std::vector<std::string> argNames;
	const std::unique_ptr<Node> block;
	
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
