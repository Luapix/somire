#include "compiler.hpp"

Context::Context(Context* parent) : parent(parent) {}

bool Context::isInner(std::string var) { return innerLocals.find(var) != innerLocals.end(); }

uint16_t Context::innerCount() { return (uint16_t) innerLocals.size(); }

void Context::define(std::string var) {
	if(isInner(var))
		throw CompileError("Trying to define " + var + " twice");
	uint16_t next = nextIndex();
	innerLocals[var] = next;
}

bool Context::isDefined(std::string var) {
	if(isInner(var)) {
		return true;
	} else if(parent) {
		return parent->isDefined(var);
	} else {
		return false;
	}
}

uint16_t Context::getIndex(std::string var) {
	if(isInner(var)) {
		return innerLocals[var];
	} else if(parent) {
		return parent->getIndex(var);
	} else {
		throw CompileError("Trying to access undefined variable " + var);
	}
}

uint16_t Context::nextIndex() {
	uint32_t next;
	if(parent) {
		next = (uint32_t) parent->nextIndex() + innerLocals.size();
	} else {
		next = innerLocals.size();
	}
	if(next >= 0xffff)
		throw CompileError("Too many locals in function");
	return (uint16_t) next;
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
		writeUI8(chunk.codeOut, (uint8_t) Opcode::POP);
		writeUI16(chunk.codeOut, ctx.innerCount());
	}
}

void Compiler::compileStatement(Chunk& chunk, Node& stat, Context& ctx) {
	switch(stat.type) {
	case NodeType::LET: {
		NodeLet& stat2 = static_cast<NodeLet&>(stat);
		ctx.define(stat2.id);
		compileExpression(chunk, *stat2.exp, ctx);
		writeUI8(chunk.codeOut, (uint8_t) Opcode::LET);
		break;
	} case NodeType::SET: {
		NodeSet& stat2 = static_cast<NodeSet&>(stat);
		compileExpression(chunk, *stat2.exp, ctx);
		writeUI8(chunk.codeOut, (uint8_t) Opcode::SET);
		writeUI16(chunk.codeOut, ctx.getIndex(stat2.id));
		break;
	} case NodeType::EXPR_STAT:
		compileExpression(chunk, *static_cast<NodeExprStat&>(stat).exp, ctx);
		writeUI8(chunk.codeOut, (uint8_t) Opcode::IGNORE);
		break;
	case NodeType::LOG:
		compileExpression(chunk, *static_cast<NodeExprStat&>(stat).exp, ctx);
		writeUI8(chunk.codeOut, (uint8_t) Opcode::LOG);
		break;
	case NodeType::IF: {
		NodeIf& stat2 = static_cast<NodeIf&>(stat);
		compileExpression(chunk, *stat2.cond, ctx);
		writeUI8(chunk.codeOut, (uint8_t) Opcode::JUMP_IF_NOT);
		uint32_t addPos = chunk.bytecode.size();
		writeI16(chunk.codeOut, 0);
		compileBlock(chunk, static_cast<NodeBlock&>(*stat2.thenBlock), &ctx);
		uint32_t addPos2;
		if(stat2.elseBlock) {
			writeUI8(chunk.codeOut, (uint8_t) Opcode::JUMP);
			addPos2 = chunk.bytecode.size();
			writeI16(chunk.codeOut, 0);
		}
		chunk.fillInJump(addPos);
		if(stat2.elseBlock) {
			compileBlock(chunk, static_cast<NodeBlock&>(*stat2.elseBlock), &ctx);
			chunk.fillInJump(addPos2);
		}
		break;
	} case NodeType::WHILE: {
		NodeWhile& stat2 = static_cast<NodeWhile&>(stat);
		uint32_t before = chunk.bytecode.size();
		compileExpression(chunk, *stat2.cond, ctx);
		writeUI8(chunk.codeOut, (uint8_t) Opcode::JUMP_IF_NOT);
		uint32_t addPos = chunk.bytecode.size();
		writeI16(chunk.codeOut, 0);
		compileBlock(chunk, static_cast<NodeBlock&>(*stat2.block), &ctx);
		writeUI8(chunk.codeOut, (uint8_t) Opcode::JUMP);
		writeI16(chunk.codeOut, computeJump(chunk.bytecode.size() + 2, before));
		chunk.fillInJump(addPos);
		break;
	} default:
		throw CompileError("Statement type not implemented: " + nodeTypeDesc(stat.type));
	}
}

std::unordered_map<std::string, Opcode> binaryOps = {
	{"+", Opcode::BIN_PLUS}, {"-", Opcode::BIN_MINUS}, {"*", Opcode::MULTIPLY}, {"/", Opcode::DIVIDE},
	{"and", Opcode::AND}, {"or", Opcode::OR}, {"==", Opcode::EQUALS}
};

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
		if(ctx.isDefined(expr2.val)) {
			writeUI8(chunk.codeOut, (uint8_t) Opcode::LOCAL);
			writeUI16(chunk.codeOut, ctx.getIndex(expr2.val));
		} else {
			writeUI8(chunk.codeOut, (uint8_t) Opcode::GLOBAL);
			writeUI16(chunk.codeOut, chunk.constants->vec.size());
			chunk.constants->vec.emplace_back(new String(expr2.val));
		}
		break;
	} case NodeType::UNI_OP: {
		NodeUnary& expr2 = static_cast<NodeUnary&>(expr);
		compileExpression(chunk, *expr2.val, ctx);
		if(expr2.op == "-") {
			writeUI8(chunk.codeOut, (uint8_t) Opcode::UNI_MINUS);
		} else if(expr2.op == "not") {
			writeUI8(chunk.codeOut, (uint8_t) Opcode::NOT);
		} else {
			throw CompileError("Unknown unary operator: " + expr2.op);
		}
		break;
	} case NodeType::BIN_OP: {
		NodeBinary& expr2 = static_cast<NodeBinary&>(expr);
		compileExpression(chunk, *expr2.left, ctx);
		compileExpression(chunk, *expr2.right, ctx);
		auto it = binaryOps.find(expr2.op);
		if(it != binaryOps.end()) {
			writeUI8(chunk.codeOut, (uint8_t) it->second);
		} else {
			throw CompileError("Unknown binary operator: " + expr2.op);
		}
		break;
	} default:
		throw CompileError("Expression type not implemented: " + nodeTypeDesc(expr.type));
	}
}

void Compiler::compileConstant(Chunk& chunk, Value val) {
	writeUI8(chunk.codeOut, (uint8_t) Opcode::CONSTANT);
	writeUI16(chunk.codeOut, (uint16_t) chunk.constants->vec.size());
	chunk.constants->vec.push_back(val);
}
