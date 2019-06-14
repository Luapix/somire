#ifndef AST_HPP
#define AST_HPP

#include <stdexcept>
#include <string>

class ParseError : public std::runtime_error {
public:
	ParseError(const std::string& what);
};


enum NodeType { N_NL, N_INDENT, N_DEDENT, N_ID };

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
	virtual ~NodeId() = default;
	
	virtual std::string toString();
	
	const std::string val;
};

#endif
