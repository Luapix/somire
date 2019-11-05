#ifndef AST_HPP
#define AST_HPP

#include <stdexcept>
#include <string>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <sstream>
#include <iomanip>
#include <memory>
#include <vector>

#include "uni_util.hpp"

class ParseError : public std::runtime_error {
public:
	ParseError(const std::string& what);
};


enum class NodeType {
	NL, INDENT, DEDENT, EOI,
	ID, INT, REAL, STR,
	SYM,
	UNI_OP, BIN_OP,
	LET, EXPR_STAT,
	BLOCK
};

std::string nodeTypeDesc(NodeType type);

class Node {
public:
	const NodeType type;
	
	Node(NodeType type);
	virtual ~Node() = default;
	
	std::string toString();
	
protected:
	virtual std::string getDataDesc();
};

class NodeId : public Node {
public:
	NodeId(std::string val);
	
	const std::string val;
	
protected:
	std::string getDataDesc() override;
};

class NodeInt : public Node {
public:
	NodeInt(std::int32_t val);
	
	const std::int32_t val;
	
protected:
	std::string getDataDesc() override;
};

class NodeReal : public Node {
public:
	NodeReal(double val);
	
	const double val;
	
protected:
	std::string getDataDesc() override;
};

class NodeString : public Node {
public:
	NodeString(std::string val);
	
	const std::string val;
	
protected:
	std::string getDataDesc() override;
};

class NodeSymbol : public Node {
public:
	NodeSymbol(std::string val);
	
	const std::string val;
	
protected:
	std::string getDataDesc() override;
};

class NodeUnary : public Node {
public:
	NodeUnary(std::string op, std::unique_ptr<Node> val);
	
	const std::string op;
	const std::unique_ptr<Node> val;
	
protected:
	std::string getDataDesc() override;
};

class NodeBinary : public Node {
public:
	NodeBinary(std::string op, std::unique_ptr<Node> left, std::unique_ptr<Node> right);
	
	const std::string op;
	const std::unique_ptr<Node> left, right;
	
protected:
	std::string getDataDesc() override;
};

class NodeLet : public Node {
public:
	NodeLet(std::string id, std::unique_ptr<Node> exp);
	
	const std::string id;
	const std::unique_ptr<Node> exp;
	
protected:
	std::string getDataDesc() override;
};

class NodeExprStat : public Node {
public:
	NodeExprStat(std::unique_ptr<Node> exp);
	
	const std::unique_ptr<Node> exp;
	
protected:
	std::string getDataDesc() override;
};

class NodeBlock : public Node {
public:
	NodeBlock(std::vector<std::unique_ptr<Node>> statements);
	
	const std::vector<std::unique_ptr<Node>> statements;
	
protected:
	std::string getDataDesc() override;
};

#endif
