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

std::unique_ptr<Chunk> Compiler::compileProgram(std::unique_ptr<Node> ast) {
	curChunk = std::unique_ptr<Chunk>(new Chunk());
	if(ast->type != NodeType::BLOCK)
		throw CompileError("Expected block to compile, got " + nodeTypeDesc(ast->type));
	compileFunction(static_cast<NodeBlock&>(*ast), std::vector<std::string>());
	return std::move(curChunk);
}

void Compiler::compileFunction(NodeBlock& block, std::vector<std::string> argNames) {
	curChunk->functions.emplace_back(new FunctionChunk());
	Context ctx;
	for(std::string arg : argNames) {
		ctx.define(arg);
	}
	compileBlock(*curChunk->functions.back(), block, ctx);
}

void Compiler::compileBlock(FunctionChunk& curFunc, NodeBlock& block, Context& ctx) {
	for(const std::unique_ptr<Node>& stat : block.statements) {
		compileStatement(curFunc, *stat, ctx);
	}
	if(ctx.innerCount() > 0) { // pop locals
		writeUI8(curFunc.codeOut, (uint8_t) Opcode::POP);
		writeUI16(curFunc.codeOut, ctx.innerCount());
	}
}

void Compiler::compileStatement(FunctionChunk& curFunc, Node& stat, Context& ctx) {
	switch(stat.type) {
	case NodeType::LET: {
		NodeLet& stat2 = static_cast<NodeLet&>(stat);
		ctx.define(stat2.id);
		compileExpression(curFunc, *stat2.exp, ctx);
		writeUI8(curFunc.codeOut, (uint8_t) Opcode::LET);
		break;
	} case NodeType::SET: {
		NodeSet& stat2 = static_cast<NodeSet&>(stat);
		compileExpression(curFunc, *stat2.exp, ctx);
		writeUI8(curFunc.codeOut, (uint8_t) Opcode::SET);
		writeUI16(curFunc.codeOut, ctx.getIndex(stat2.id));
		break;
	} case NodeType::EXPR_STAT:
		compileExpression(curFunc, *static_cast<NodeExprStat&>(stat).exp, ctx);
		writeUI8(curFunc.codeOut, (uint8_t) Opcode::IGNORE);
		break;
	case NodeType::IF: {
		NodeIf& stat2 = static_cast<NodeIf&>(stat);
		compileExpression(curFunc, *stat2.cond, ctx);
		writeUI8(curFunc.codeOut, (uint8_t) Opcode::JUMP_IF_NOT);
		uint32_t addPos = curFunc.code.size();
		writeI16(curFunc.codeOut, 0);
		Context thenCtx(&ctx);
		compileBlock(curFunc, static_cast<NodeBlock&>(*stat2.thenBlock), thenCtx);
		uint32_t addPos2;
		if(stat2.elseBlock) {
			writeUI8(curFunc.codeOut, (uint8_t) Opcode::JUMP);
			addPos2 = curFunc.code.size();
			writeI16(curFunc.codeOut, 0);
		}
		curFunc.fillInJump(addPos);
		if(stat2.elseBlock) {
			Context elseCtx(&ctx);
			compileBlock(curFunc, static_cast<NodeBlock&>(*stat2.elseBlock), elseCtx);
			curFunc.fillInJump(addPos2);
		}
		break;
	} case NodeType::WHILE: {
		NodeWhile& stat2 = static_cast<NodeWhile&>(stat);
		uint32_t before = curFunc.code.size();
		compileExpression(curFunc, *stat2.cond, ctx);
		writeUI8(curFunc.codeOut, (uint8_t) Opcode::JUMP_IF_NOT);
		uint32_t addPos = curFunc.code.size();
		writeI16(curFunc.codeOut, 0);
		Context innerCtx(&ctx);
		compileBlock(curFunc, static_cast<NodeBlock&>(*stat2.block), innerCtx);
		writeUI8(curFunc.codeOut, (uint8_t) Opcode::JUMP);
		writeI16(curFunc.codeOut, computeJump(curFunc.code.size() + 2, before));
		curFunc.fillInJump(addPos);
		break;
	} case NodeType::RETURN:
		compileExpression(curFunc, *static_cast<NodeReturn&>(stat).expr, ctx);
		writeUI8(curFunc.codeOut, (uint8_t) Opcode::RETURN);
		break;
	default:
		throw CompileError("Statement type not implemented: " + nodeTypeDesc(stat.type));
	}
}

std::unordered_map<std::string, Opcode> binaryOps = {
	{"+", Opcode::BIN_PLUS}, {"-", Opcode::BIN_MINUS}, {"*", Opcode::MULTIPLY}, {"/", Opcode::DIVIDE},
	{"and", Opcode::AND}, {"or", Opcode::OR},
	{"==", Opcode::EQUALS}, {"<", Opcode::LESS}, {"<=", Opcode::LESS_OR_EQ}
};

void Compiler::compileExpression(FunctionChunk& curFunc, Node& expr, Context& ctx) {
	switch(expr.type) {
	case NodeType::INT:
		compileConstant(curFunc, Value(static_cast<NodeInt&>(expr).val));
		break;
	case NodeType::REAL:
		compileConstant(curFunc, Value(static_cast<NodeReal&>(expr).val));
		break;
	case NodeType::STR:
		compileConstant(curFunc, Value(new String(static_cast<NodeString&>(expr).val)));
		break;
	case NodeType::SYM: {
		NodeSymbol& expr2 = static_cast<NodeSymbol&>(expr);
		if(expr2.val == "nil") {
			compileConstant(curFunc, Value());
		} else if(expr2.val == "true") {
			compileConstant(curFunc, Value(true));
		} else if(expr2.val == "false") {
			compileConstant(curFunc, Value(false));
		} else {
			throw CompileError("Unexpected keyword in expression: " + expr2.val);
		}
		break;
	} case NodeType::ID: {
		NodeId& expr2 = static_cast<NodeId&>(expr);
		if(ctx.isDefined(expr2.val)) {
			writeUI8(curFunc.codeOut, (uint8_t) Opcode::LOCAL);
			writeUI16(curFunc.codeOut, ctx.getIndex(expr2.val));
		} else {
			writeUI8(curFunc.codeOut, (uint8_t) Opcode::GLOBAL);
			writeUI16(curFunc.codeOut, curChunk->constants->vec.size());
			curChunk->constants->vec.emplace_back(new String(expr2.val));
		}
		break;
	} case NodeType::UNI_OP: {
		NodeUnary& expr2 = static_cast<NodeUnary&>(expr);
		compileExpression(curFunc, *expr2.val, ctx);
		if(expr2.op == "-") {
			writeUI8(curFunc.codeOut, (uint8_t) Opcode::UNI_MINUS);
		} else if(expr2.op == "not") {
			writeUI8(curFunc.codeOut, (uint8_t) Opcode::NOT);
		} else {
			throw CompileError("Unknown unary operator: " + expr2.op);
		}
		break;
	} case NodeType::BIN_OP: {
		NodeBinary& expr2 = static_cast<NodeBinary&>(expr);
		compileExpression(curFunc, *expr2.left, ctx);
		compileExpression(curFunc, *expr2.right, ctx);
		auto it = binaryOps.find(expr2.op);
		if(it != binaryOps.end()) {
			writeUI8(curFunc.codeOut, (uint8_t) it->second);
		} else if(expr2.op == "!=") {
			writeUI8(curFunc.codeOut, (uint8_t) Opcode::EQUALS);
			writeUI8(curFunc.codeOut, (uint8_t) Opcode::NOT);
		} else if(expr2.op == ">") {
			writeUI8(curFunc.codeOut, (uint8_t) Opcode::LESS_OR_EQ);
			writeUI8(curFunc.codeOut, (uint8_t) Opcode::NOT);
		} else if(expr2.op == ">=") {
			writeUI8(curFunc.codeOut, (uint8_t) Opcode::LESS);
			writeUI8(curFunc.codeOut, (uint8_t) Opcode::NOT);
		} else {
			throw CompileError("Unknown binary operator: " + expr2.op);
		}
		break;
	} case NodeType::CALL: {
		NodeCall& expr2 = static_cast<NodeCall&>(expr);
		for(auto& arg : expr2.args) {
			compileExpression(curFunc, *arg, ctx);
		}
		compileExpression(curFunc, *expr2.func, ctx);
		writeUI8(curFunc.codeOut, (uint8_t) Opcode::CALL);
		writeUI16(curFunc.codeOut, (uint16_t) expr2.args.size());
		break;
	} case NodeType::FUNC: {
		NodeFunction& expr2 = static_cast<NodeFunction&>(expr);
		writeUI8(curFunc.codeOut, (uint8_t) Opcode::MAKE_FUNC);
		if(curChunk->functions.size() > 0xffff)
			throw CompileError("Too many functions in program");
		writeUI16(curFunc.codeOut, (uint16_t) curChunk->functions.size());
		if(expr2.argNames.size() > 0xffff)
			throw CompileError("Too many arguments in function definition");
		writeUI16(curFunc.codeOut, (uint16_t) expr2.argNames.size());
		compileFunction(static_cast<NodeBlock&>(*expr2.block), expr2.argNames);
		break;
	} default:
		throw CompileError("Expression type not implemented: " + nodeTypeDesc(expr.type));
	}
}

void Compiler::compileConstant(FunctionChunk& curFunc, Value val) {
	writeUI8(curFunc.codeOut, (uint8_t) Opcode::CONSTANT);
	writeUI16(curFunc.codeOut, (uint16_t) curChunk->constants->vec.size());
	curChunk->constants->vec.push_back(val);
}
