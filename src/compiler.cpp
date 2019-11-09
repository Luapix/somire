#include "compiler.hpp"

CompileError::CompileError(const std::string& what)
	: runtime_error("Compile error: " + what) { }

Compiler::Compiler() {}

std::unique_ptr<Chunk> Compiler::compileChunk(std::unique_ptr<Node> ast) {
	std::unique_ptr<Chunk> chunk(new Chunk());
	if(ast->type != NodeType::BLOCK)
		throw CompileError("Expected block to compile, got " + nodeTypeDesc(ast->type));
	compileBlock(*chunk, static_cast<NodeBlock&>(*ast));
	return chunk;
}

void Compiler::compileBlock(Chunk& chunk, NodeBlock& block) {
	for(const std::unique_ptr<Node>& stat : block.statements) {
		compileStatement(chunk, *stat);
	}
}

void Compiler::compileStatement(Chunk& chunk, Node& stat) {
	switch(stat.type) {
	case NodeType::EXPR_STAT:
		compileExpression(chunk, *static_cast<NodeExprStat&>(stat).exp);
		break;
	default:
		throw CompileError("Statement type not implemented: " + nodeTypeDesc(stat.type));
	}
}

void Compiler::compileExpression(Chunk& chunk, Node& expr) {
	switch(expr.type) {
	case NodeType::INT:
		compileConstant(chunk, new ValueInt(static_cast<NodeInt&>(expr).val));
		break;
	case NodeType::SYM: {
		NodeSymbol& expr2 = static_cast<NodeSymbol&>(expr);
		if(expr2.val == "nil") {
			compileConstant(chunk, new Value(ValueType::NIL));
		} else {
			throw CompileError("Unexpected keyword in expression: " + expr2.val);
		}
		break;
	} case NodeType::UNI_OP: {
		NodeUnary& expr2 = static_cast<NodeUnary&>(expr);
		compileExpression(chunk, *expr2.val);
		if(expr2.op == "-") {
			chunk.bytecode.push_back((uint8_t) Opcode::UNI_MINUS);
		} else {
			throw CompileError("Unknown unary operator: " + expr2.op);
		}
		break;
	} case NodeType::BIN_OP: {
		NodeBinary& expr2 = static_cast<NodeBinary&>(expr);
		compileExpression(chunk, *expr2.left);
		compileExpression(chunk, *expr2.right);
		if(expr2.op == "+") {
			chunk.bytecode.push_back((uint8_t) Opcode::BIN_PLUS);
		} else {
			throw CompileError("Unknown binary operator: " + expr2.op);
		}
		break;
	} default:
		throw CompileError("Expression type not implemented: " + nodeTypeDesc(expr.type));
	}
}

void Compiler::compileConstant(Chunk& chunk, Value* val) {
	chunk.bytecode.push_back((uint8_t) Opcode::CONSTANT);
	chunk.bytecode.push_back((uint8_t) chunk.constants->vec.size());
	chunk.constants->vec.push_back(val);
}
