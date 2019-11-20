#include "compiler.hpp"

Context::Context(bool isFuncTop, Context* parent)
	: isFuncTop(isFuncTop), parent(parent), nextLocal(0), nextUpvalue(-1), innerLocalCount(0) {
	if(!isFuncTop) {
		nextLocal = parent->nextLocal;
	}
}

bool Context::getVariable(std::string var, int16_t& idx) {
	auto it = variables.find(var);
	if(it != variables.end()) { // if local or known upvalue
		idx = it->second;
		return true;
	} else if(isFuncTop) { // look in surrounding function
		if(parent) {
			int16_t idx2;
			if(parent->getVariable(var, idx2)) { // add new upvalue
				idx = nextUpvalue--;
				variables[var] = idx;
				upvalues.push_back(idx2); // save what the upvalue points to
				return true;
			} else { // global or undefined
				return false;
			}
		} else { // global or undefined
			return false;
		}
	} else {
		return parent->getVariable(var, idx);
	}
}

void Context::defineLocal(std::string var) {
	variables[var] = nextLocal++;
	innerLocalCount++;
}

uint16_t Context::getLocalCount() {
	return innerLocalCount;
}

std::vector<int16_t>& Context::getFunctionUpvalues() {
	if(isFuncTop) {
		return upvalues;
	} else {
		return parent->getFunctionUpvalues();
	}
}


Compiler::Compiler() {}

std::unique_ptr<Chunk> Compiler::compileProgram(std::unique_ptr<Node> ast) {
	curChunk = std::unique_ptr<Chunk>(new Chunk());
	if(ast->type != NodeType::BLOCK)
		throw CompileError("Expected block to compile, got " + nodeTypeDesc(ast->type));
	compileFunction(static_cast<NodeBlock&>(*ast), std::vector<std::string>());
	return std::move(curChunk);
}

std::vector<int16_t> Compiler::compileFunction(NodeBlock& block, std::vector<std::string> argNames, Context* parent) {
	curChunk->functions.emplace_back(new FunctionChunk());
	Context ctx(true, parent);
	for(std::string arg : argNames) {
		ctx.defineLocal(arg);
	}
	compileBlock(*curChunk->functions.back(), block, ctx, false); // no need to pop locals at the end of a function
	return ctx.getFunctionUpvalues();
}

void Compiler::compileBlock(FunctionChunk& curFunc, NodeBlock& block, Context& ctx, bool popLocals) {
	for(const std::unique_ptr<Node>& stat : block.statements) {
		compileStatement(curFunc, *stat, ctx);
	}
	if(popLocals && ctx.getLocalCount() > 0) { // pop locals
		writeUI8(curFunc.codeOut, (uint8_t) Opcode::POP);
		writeUI16(curFunc.codeOut, ctx.getLocalCount());
	}
}

void Compiler::compileStatement(FunctionChunk& curFunc, Node& stat, Context& ctx) {
	switch(stat.type) {
	case NodeType::LET: {
		NodeLet& stat2 = static_cast<NodeLet&>(stat);
		if(stat2.exp->type == NodeType::FUNC)
			ctx.defineLocal(stat2.id); // Define in advance to allow for recursion
		compileExpression(curFunc, *stat2.exp, ctx);
		writeUI8(curFunc.codeOut, (uint8_t) Opcode::LET);
		if(stat2.exp->type != NodeType::FUNC)
			ctx.defineLocal(stat2.id);
		break;
	} case NodeType::SET: {
		NodeSet& stat2 = static_cast<NodeSet&>(stat);
		int16_t idx;
		if(!ctx.getVariable(stat2.id, idx))
			throw CompileError("Trying to set global or undefined variable " + stat2.id); // for now, no modifying globals
		compileExpression(curFunc, *stat2.exp, ctx);
		writeUI8(curFunc.codeOut, (uint8_t) Opcode::SET_LOCAL);
		writeI16(curFunc.codeOut, idx);
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
		Context thenCtx(false, &ctx);
		compileBlock(curFunc, static_cast<NodeBlock&>(*stat2.thenBlock), thenCtx);
		uint32_t addPos2;
		if(stat2.elseBlock) {
			writeUI8(curFunc.codeOut, (uint8_t) Opcode::JUMP);
			addPos2 = curFunc.code.size();
			writeI16(curFunc.codeOut, 0);
		}
		curFunc.fillInJump(addPos);
		if(stat2.elseBlock) {
			Context elseCtx(false, &ctx);
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
		Context innerCtx(false, &ctx);
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
	{"+", Opcode::BIN_PLUS}, {"-", Opcode::BIN_MINUS}, {"*", Opcode::MULTIPLY}, {"/", Opcode::DIVIDE}, {"%", Opcode::MODULO},
	{"^", Opcode::POWER},
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
		int16_t idx;
		if(ctx.getVariable(expr2.val, idx)) {
			writeUI8(curFunc.codeOut, (uint8_t) Opcode::LOCAL);
			writeI16(curFunc.codeOut, idx);
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
		std::vector<int16_t> upvalues = compileFunction(static_cast<NodeBlock&>(*expr2.block), expr2.argNames, &ctx);
		if(upvalues.size() > 0xffff)
			throw CompileError("Too many upvalues in function definition");
		writeUI16(curFunc.codeOut, (uint16_t) upvalues.size());
		for(int16_t upvalue : upvalues) {
			writeI16(curFunc.codeOut, upvalue);
		}
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
