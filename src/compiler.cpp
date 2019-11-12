#include "compiler.hpp"

CompileError::CompileError(const std::string& what)
	: runtime_error("Compile error: " + what) { }

Context::Context(Context* parent) : parent(parent) {}

bool Context::isInner(std::string var) { return innerLocals.find(var) != innerLocals.end(); }

uint8_t Context::innerCount() { return (uint8_t) innerLocals.size(); }

void Context::define(std::string var) {
	if(isInner(var))
		throw CompileError("Trying to define " + var + " twice");
	uint8_t next = nextIndex();
	innerLocals[var] = next;
}

uint8_t Context::getIndex(std::string var) {
	if(isInner(var)) {
		return innerLocals[var];
	} else if(parent) {
		return parent->getIndex(var);
	} else {
		throw CompileError("Trying to access undefined variable " + var);
	}
}

uint8_t Context::nextIndex() {
	uint32_t next;
	if(parent) {
		next = (uint32_t) parent->nextIndex() + innerLocals.size();
	} else {
		next = innerLocals.size();
	}
	if(next >= 0xff)
		throw CompileError("Too many locals in function");
	return (uint8_t) next;
}

Compiler::Compiler() {}

std::unique_ptr<Chunk> Compiler::compileChunk(std::unique_ptr<Node> ast) {
	std::unique_ptr<Chunk> chunk(new Chunk());
	if(ast->type != NodeType::BLOCK)
		throw CompileError("Expected block to compile, got " + nodeTypeDesc(ast->type));
	compileBlock(*chunk, static_cast<NodeBlock&>(*ast));
	return chunk;
}

void Compiler::compileBlock(Chunk& chunk, NodeBlock& block, Context* parent) {
	Context ctx(parent);
	for(const std::unique_ptr<Node>& stat : block.statements) {
		compileStatement(chunk, *stat, ctx);
	}
	if(ctx.innerCount() > 0) { // pop locals
		chunk.writeOpcode(Opcode::POP);
		chunk.writeUI8(ctx.innerCount());
	}
}

void Compiler::compileStatement(Chunk& chunk, Node& stat, Context& ctx) {
	switch(stat.type) {
	case NodeType::LET: {
		NodeLet& stat2 = static_cast<NodeLet&>(stat);
		ctx.define(stat2.id);
		compileExpression(chunk, *stat2.exp, ctx);
		chunk.writeOpcode(Opcode::LET);
		break;
	} case NodeType::SET: {
		NodeSet& stat2 = static_cast<NodeSet&>(stat);
		compileExpression(chunk, *stat2.exp, ctx);
		chunk.writeOpcode(Opcode::SET);
		chunk.writeUI8(ctx.getIndex(stat2.id));
		break;
	} case NodeType::EXPR_STAT:
		compileExpression(chunk, *static_cast<NodeExprStat&>(stat).exp, ctx);
		chunk.writeOpcode(Opcode::IGNORE);
		break;
	case NodeType::LOG:
		compileExpression(chunk, *static_cast<NodeExprStat&>(stat).exp, ctx);
		chunk.writeOpcode(Opcode::LOG);
		break;
	case NodeType::IF: {
		NodeIf& stat2 = static_cast<NodeIf&>(stat);
		compileExpression(chunk, *stat2.cond, ctx);
		chunk.writeOpcode(Opcode::JUMP_IF_NOT);
		chunk.writeUI8(0); // placeholder
		uint32_t priorAdd = chunk.bytecode.size();
		compileBlock(chunk, static_cast<NodeBlock&>(*stat2.block), &ctx);
		uint32_t relJump = chunk.bytecode.size() - (int32_t) priorAdd;
		if(relJump > 0xff) throw CompileError("Can't jump more than 255 instructions away");
		chunk.bytecode[priorAdd-1] = (uint8_t) relJump;
		break;
	} default:
		throw CompileError("Statement type not implemented: " + nodeTypeDesc(stat.type));
	}
}

void Compiler::compileExpression(Chunk& chunk, Node& expr, Context& ctx) {
	switch(expr.type) {
	case NodeType::INT:
		compileConstant(chunk, Value(static_cast<NodeInt&>(expr).val));
		break;
	case NodeType::REAL:
		compileConstant(chunk, Value(static_cast<NodeReal&>(expr).val));
		break;
	case NodeType::STR:
		compileConstant(chunk, Value(new String(static_cast<NodeString&>(expr).val)));
		break;
	case NodeType::SYM: {
		NodeSymbol& expr2 = static_cast<NodeSymbol&>(expr);
		if(expr2.val == "nil") {
			compileConstant(chunk, Value());
		} else if(expr2.val == "true") {
			compileConstant(chunk, Value(true));
		} else if(expr2.val == "false") {
			compileConstant(chunk, Value(false));
		} else {
			throw CompileError("Unexpected keyword in expression: " + expr2.val);
		}
		break;
	} case NodeType::ID: {
		NodeId& expr2 = static_cast<NodeId&>(expr);
		chunk.writeOpcode(Opcode::LOCAL);
		chunk.writeUI8(ctx.getIndex(expr2.val));
		break;
	} case NodeType::UNI_OP: {
		NodeUnary& expr2 = static_cast<NodeUnary&>(expr);
		compileExpression(chunk, *expr2.val, ctx);
		if(expr2.op == "-") {
			chunk.writeOpcode(Opcode::UNI_MINUS);
		} else if(expr2.op == "not") {
			chunk.writeOpcode(Opcode::NOT);
		} else {
			throw CompileError("Unknown unary operator: " + expr2.op);
		}
		break;
	} case NodeType::BIN_OP: {
		NodeBinary& expr2 = static_cast<NodeBinary&>(expr);
		compileExpression(chunk, *expr2.left, ctx);
		compileExpression(chunk, *expr2.right, ctx);
		if(expr2.op == "+") {
			chunk.writeOpcode(Opcode::BIN_PLUS);
		} else if(expr2.op == "and") {
			chunk.writeOpcode(Opcode::AND);
		} else if(expr2.op == "or") {
			chunk.writeOpcode(Opcode::OR);
		} else if(expr2.op == "==") {
			chunk.writeOpcode(Opcode::EQUALS);
		} else {
			throw CompileError("Unknown binary operator: " + expr2.op);
		}
		break;
	} default:
		throw CompileError("Expression type not implemented: " + nodeTypeDesc(expr.type));
	}
}

void Compiler::compileConstant(Chunk& chunk, Value val) {
	chunk.writeOpcode(Opcode::CONSTANT);
	chunk.writeUI8((uint8_t) chunk.constants->vec.size());
	chunk.constants->vec.push_back(val);
}
