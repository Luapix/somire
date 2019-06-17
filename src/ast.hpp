#ifndef AST_HPP
#define AST_HPP

#include <stdexcept>
#include <string>
#include <cstdint>
#include <cstdio>
#include <cassert>

class ParseError : public std::runtime_error {
public:
	ParseError(const std::string& what);
};


enum NodeType { N_NL, N_INDENT, N_DEDENT, N_ID, N_INT, N_REAL };

class Node {
public:
	Node(NodeType type);
	virtual ~Node() = default;
	
	virtual std::string toString();
	
	const NodeType type;
};

class NodeId : public Node {
public:
	NodeId(std::string val);
	
	std::string toString() override;
	
	const std::string val;
};

class NodeInt : public Node {
public:
	NodeInt(std::int32_t val);
	
	std::string toString() override;
	
	const std::int32_t val;
};

class NodeReal : public Node {
public:
	NodeReal(double val);
	
	std::string toString() override;
	
	const double val;
};

#endif
