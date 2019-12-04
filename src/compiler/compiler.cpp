#include "compiler.hpp"

#include <unordered_set>

Context::Context(bool isFuncTop, Context* parent)
	: isFuncTop(isFuncTop), parent(parent), nextLocal(0), nextUpvalue(-1), innerLocalCount(0) {
	if(!isFuncTop) {
		nextLocal = parent->nextLocal;
	}
}

std::optional<Variable> Context::getVariable(std::string varName) {
	auto it = variables.find(varName);
	if(it != variables.end()) { // if local or known upvalue
		return it->second;
	} else if(isFuncTop) { // look in surrounding function
		if(parent) {
			std::optional<Variable> var = parent->getVariable(varName);
			if(var) { // add new upvalue
				Variable upvalue = { nextUpvalue--, var->type };
				variables[varName] = upvalue;
				upvalues.push_back(var->idx); // save what the upvalue points to
				return upvalue;
			} else { // global or undefined
				return {};
			}
		} else { // global or undefined
			return {};
		}
	} else {
		return parent->getVariable(varName);
	}
}

void Context::defineLocal(std::string var, Type* type) {
	variables[var] = { nextLocal++, type };
	innerLocalCount++;
}

void Context::changeType(std::string var, Type* type) {
	variables[var].type = type;
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


Compiler::Compiler() : types(new TypeNamespace()), globals(new TypeNamespace()) {
	defineBasicTypes(*types);
	anyType = types->map["any"];
	nilType = types->map["nil"];
	boolType = types->map["bool"];
	realType = types->map["real"];
	intType = types->map["int"];
	stringType = types->map["string"];
	macroType = types->map["macro"];
	defineStdTypes(*globals, *types);
}

std::unique_ptr<Chunk> Compiler::compileProgram(Node* ast) {
	curChunk = std::unique_ptr<Chunk>(new Chunk());
	if(ast->type != NodeType::BLOCK)
		throw CompileError("Expected block to compile, got " + nodeTypeDesc(ast->type));
	Type* resType = nullptr;
	compileFunction(static_cast<NodeBlock&>(*ast), {}, {}, &resType);
	return std::move(curChunk);
}

Type* Compiler::getType(Node& type) {
	auto simpleType = dynamic_cast<NodeSimpleType*>(&type);
	if(simpleType) {
		auto it = types->map.find(simpleType->name);
		if(it != types->map.end()) {
			return it->second;
		} else {
			throw CompileError("Unknown type " + simpleType->name);
		}
	} else {
		throw CompileError("Invalid node for type constraint: " + nodeTypeDesc(type.type));
	}
}

std::vector<int16_t> Compiler::compileFunction(NodeBlock& block, std::vector<std::string> argNames, std::vector<Type*> argTypes, Type** resType, Context* parent) {
	curChunk->functions.emplace_back(new FunctionChunk());
	Context ctx(true, parent);
	for(uint32_t i = 0; i < argNames.size(); i++) {
		ctx.defineLocal(argNames[i], argTypes[i]);
	}
	compileBlock(*curChunk->functions.back(), block, ctx, resType, true);
	return ctx.getFunctionUpvalues();
}

void Compiler::compileBlock(FunctionChunk& curFunc, NodeBlock& block, Context& ctx, Type** resType, bool mainBlock) {
	for(Node* stat : block.statements) {
		compileStatement(curFunc, *stat, ctx, resType);
	}
	if(!mainBlock && ctx.getLocalCount() > 0) { // pop locals
		writeUI8(curFunc.codeOut, (uint8_t) Opcode::POP);
		writeUI16(curFunc.codeOut, ctx.getLocalCount());
	}
	if(mainBlock &&
	   (block.statements.size() == 0 || block.statements.back()->type != NodeType::RETURN)) {
	   	// implicit return
		*resType = nilType;
	}
}

void Compiler::compileStatement(FunctionChunk& curFunc, Node& stat, Context& ctx, Type** resType) {
	switch(stat.type) {
	case NodeType::LET: {
		NodeLet& stat2 = static_cast<NodeLet&>(stat);
		if(stat2.exp->type == NodeType::FUNC) // Define in advance to allow for recursion
			ctx.defineLocal(stat2.id, macroType); // To be improved
		Type* valType = compileExpression(curFunc, *stat2.exp, ctx);
		writeUI8(curFunc.codeOut, (uint8_t) Opcode::LET);
		if(stat2.exp->type == NodeType::FUNC)
			ctx.changeType(stat2.id, valType);
		else
			ctx.defineLocal(stat2.id, valType);
		break;
	} case NodeType::SET: {
		NodeSet& stat2 = static_cast<NodeSet&>(stat);
		std::optional<Variable> var = ctx.getVariable(stat2.id);
		if(!var)
			throw CompileError("Trying to set global or undefined variable " + stat2.id); // for now, no modifying globals
		Type* valType = compileExpression(curFunc, *stat2.exp, ctx);
		if(!valType->canBeAssignedTo(var->type))
			throw CompileError("Trying to set variable of type " + var->type->getDesc() + " to value of type " + valType->getDesc());
		writeUI8(curFunc.codeOut, (uint8_t) Opcode::SET_LOCAL);
		writeI16(curFunc.codeOut, var->idx);
		break;
	} case NodeType::EXPR_STAT:
		compileExpression(curFunc, *static_cast<NodeExprStat&>(stat).exp, ctx);
		writeUI8(curFunc.codeOut, (uint8_t) Opcode::IGNORE);
		break;
	case NodeType::IF: {
		NodeIf& stat2 = static_cast<NodeIf&>(stat);
		Type* condType = compileExpression(curFunc, *stat2.cond, ctx);
		if(!condType->canBeAssignedTo(boolType))
			throw CompileError("Expecting boolean in condition, got value of type " + condType->getDesc());
		writeUI8(curFunc.codeOut, (uint8_t) Opcode::JUMP_IF_NOT);
		uint32_t addPos = curFunc.code.size();
		writeI16(curFunc.codeOut, 0);
		Context thenCtx(false, &ctx);
		compileBlock(curFunc, static_cast<NodeBlock&>(*stat2.thenBlock), thenCtx, resType);
		uint32_t addPos2;
		if(stat2.elseBlock) {
			writeUI8(curFunc.codeOut, (uint8_t) Opcode::JUMP);
			addPos2 = curFunc.code.size();
			writeI16(curFunc.codeOut, 0);
		}
		curFunc.fillInJump(addPos);
		if(stat2.elseBlock) {
			Context elseCtx(false, &ctx);
			compileBlock(curFunc, static_cast<NodeBlock&>(*stat2.elseBlock), elseCtx, resType);
			curFunc.fillInJump(addPos2);
		}
		break;
	} case NodeType::WHILE: {
		NodeWhile& stat2 = static_cast<NodeWhile&>(stat);
		uint32_t before = curFunc.code.size();
		Type* condType = compileExpression(curFunc, *stat2.cond, ctx);
		if(!condType->canBeAssignedTo(boolType))
			throw CompileError("Expecting boolean in while loop, got value of type " + condType->getDesc());
		writeUI8(curFunc.codeOut, (uint8_t) Opcode::JUMP_IF_NOT);
		uint32_t addPos = curFunc.code.size();
		writeI16(curFunc.codeOut, 0);
		Context innerCtx(false, &ctx);
		compileBlock(curFunc, static_cast<NodeBlock&>(*stat2.block), innerCtx, resType);
		writeUI8(curFunc.codeOut, (uint8_t) Opcode::JUMP);
		writeI16(curFunc.codeOut, computeJump(curFunc.code.size() + 2, before));
		curFunc.fillInJump(addPos);
		break;
	} case NodeType::RETURN: {
		Type* resType2 = compileExpression(curFunc, *static_cast<NodeReturn&>(stat).exp, ctx);
		writeUI8(curFunc.codeOut, (uint8_t) Opcode::RETURN);
		if(*resType) {
			if((*resType)->canBeAssignedTo(resType2)) {
				*resType = resType2;
			} else if(!resType2->canBeAssignedTo(*resType)) {
				throw CompileError("Returning " + (*resType)->getDesc() + " somewhere, and " + resType2->getDesc() + " somewhere else");
			}
		} else {
			*resType = resType2;
		}
		break;
	} default:
		throw CompileError("Statement type not implemented: " + nodeTypeDesc(stat.type));
	}
}

std::unordered_set<std::string> numericOps = {"+", "-", "*", "%"}; // int ⋅ int → int ; real ⋅ real → real
std::unordered_set<std::string> comparisonOps = {"<", ">", "<=", ">="};

std::unordered_map<std::string, Opcode> binaryOps = {
	{"+", Opcode::BIN_PLUS}, {"-", Opcode::BIN_MINUS}, {"*", Opcode::MULTIPLY}, {"/", Opcode::DIVIDE}, {"%", Opcode::MODULO},
	{"^", Opcode::POWER},
	{"and", Opcode::AND}, {"or", Opcode::OR},
	{"==", Opcode::EQUALS}, {"<", Opcode::LESS}, {"<=", Opcode::LESS_OR_EQ},
	{"index", Opcode::INDEX}
};

Type* Compiler::typeExpression(NodeExp& exp, Context& ctx) {
	switch(exp.type) {
	case NodeType::INT:
		return exp.valueType = intType;
	case NodeType::REAL:
		return exp.valueType = realType;
	case NodeType::STR:
		return exp.valueType = stringType;
	case NodeType::SYM: {
		NodeSymbol& exp2 = static_cast<NodeSymbol&>(exp);
		if(exp2.val == "nil") {
			return exp.valueType = nilType;
		} else if(exp2.val == "true" || exp2.val == "false") {
			return exp.valueType = boolType;
		} else {
			throw CompileError("Unexpected keyword in expression: " + exp2.val);
		}
	} case NodeType::ID: {
		NodeId& exp2 = static_cast<NodeId&>(exp);
		std::optional<Variable> var = ctx.getVariable(exp2.val);
		if(var) {
			return exp.valueType = var->type;
		} else {
			auto it = globals->map.find(exp2.val);
			if(it != globals->map.end()) {
				return exp.valueType = it->second;
			} else {
				throw CompileError("Trying to access unknown variable: " + exp2.val);
			}
		}
	} case NodeType::UNI_OP: {
		NodeUnary& exp2 = static_cast<NodeUnary&>(exp);
		Type* valType = typeExpression(*exp2.val, ctx);
		if(exp2.op == "-") {
			return exp.valueType = valType;
		} else if(exp2.op == "not") {
			return exp.valueType = boolType;
		} else {
			throw CompileError("Unknown unary operator: " + exp2.op);
		}
	} case NodeType::BIN_OP: {
		NodeBinary& exp2 = static_cast<NodeBinary&>(exp);
		Type* type1 = typeExpression(*exp2.left, ctx);
		Type* type2 = typeExpression(*exp2.right, ctx);
		bool i1 = type1->canBeAssignedTo(intType),  i2 = type2->canBeAssignedTo(intType),
		     r1 = type1->canBeAssignedTo(realType), r2 = type2->canBeAssignedTo(realType);
		if(numericOps.find(exp2.op) != numericOps.end()) {
			if(i1 && i2) {
				return exp.valueType = intType;
			} else if(r1 && r2) {
				return exp.valueType = realType;
			} else {
				throw CompileError("Trying to perform arithmetic on " + type1->getDesc() + " and " + type2->getDesc());
			}
		} else if(comparisonOps.find(exp2.op) != comparisonOps.end()) {
			if(r1 && r2) {
				return exp.valueType = boolType;
			} else {
				throw CompileError("Trying to compare " + type1->getDesc() + " and " + type2->getDesc());
			}
		} else if(exp2.op == "==" || exp2.op == "!=") {
			return exp.valueType = boolType;
		} else if(exp2.op == "/" || exp2.op == "^") {
			if(r1 && r2) {
				return exp.valueType = realType;
			} else {
				throw CompileError("Trying to perform real operations on " + type1->getDesc() + " and " + type2->getDesc());
			}
		} else if(exp2.op == "and" || exp2. op == "or") {
			if(type1->canBeAssignedTo(boolType) && type2->canBeAssignedTo(boolType)) {
				return exp.valueType = boolType;
			} else {
				throw CompileError("Trying to perform boolean operations on " + type1->getDesc() + " and " + type2->getDesc());
			}
		} else if(exp2.op == "index") {
			ListType* listType = dynamic_cast<ListType*>(type1);
			if(listType && listType->elemType && type2->canBeAssignedTo(intType)) {
				return exp.valueType = listType->elemType;
			} else {
				throw CompileError("Trying to index " + type1->getDesc() + " with " + type2->getDesc());
			}
		}
		throw CompileError("Type deduction not implemented for operator " + exp2.op);
	} case NodeType::CALL: {
		NodeCall& exp2 = static_cast<NodeCall&>(exp);
		std::vector<Type*> argTypes;
		for(auto& arg : exp2.args) {
			argTypes.push_back(typeExpression(*arg, ctx));
		}
		Type* funcType = typeExpression(*exp2.func, ctx);
		
		FunctionType* funcType2;
		if(funcType->canBeAssignedTo(macroType)) {
			return anyType;
		} else if(funcType2 = dynamic_cast<FunctionType*>(funcType)) {
			uint32_t expected = funcType2->argTypes.size();
			uint32_t got = argTypes.size();
			if(got != expected)
				throw CompileError("Expected " + std::to_string(expected) + " arguments in function call, got " + std::to_string(got));
			for(uint32_t i = 0; i < got; i++) {
				if(!argTypes[i]->canBeAssignedTo(funcType2->argTypes[i]))
					throw CompileError("Cannot assign " + argTypes[i]->getDesc() + " to " + funcType2->argTypes[i]->getDesc() + " argument");
			}
			return exp.valueType = funcType2->resType;
		} else {
			throw CompileError("Trying to call " + funcType->getDesc());
		}
	} case NodeType::FUNC: {
		NodeFunction& exp2 = static_cast<NodeFunction&>(exp);
		exp2.protoIdx = curChunk->functions.size();
		std::vector<Type*> argTypes;
		for(auto& argTypeDesc : exp2.argTypes) {
			argTypes.push_back(getType(*argTypeDesc));
		}
		Type* resType = nullptr;
		exp2.upvalues = compileFunction(static_cast<NodeBlock&>(*exp2.block), exp2.argNames, argTypes, &resType, &ctx);
		return exp.valueType = new FunctionType(argTypes, resType);
	} case NodeType::LIST: {
		NodeList& exp2 = static_cast<NodeList&>(exp);
		Type* elemType = nullptr;
		for(NodeExp* val : exp2.vals) {
			Type* elemType2 = typeExpression(*val, ctx);
			if(elemType) {
				if(elemType->canBeAssignedTo(elemType2)) {
					elemType = elemType2;
				} else if(!elemType2->canBeAssignedTo(elemType)) {
					throw CompileError("Cannot mix " + elemType->getDesc() + " and " + elemType2->getDesc() + " in list literal");
				}
			} else {
				elemType = elemType2;
			}
		}
		return exp.valueType = new ListType(elemType);
	} default:
		throw CompileError("Expression type not implemented: " + nodeTypeDesc(exp.type));
	}
}

Type* Compiler::compileExpression(FunctionChunk& curFunc, Node& expr, Context& ctx) {
	switch(expr.type) {
	case NodeType::INT:
		compileConstant(curFunc, Value(static_cast<NodeInt&>(expr).val));
		return intType;
	case NodeType::REAL:
		compileConstant(curFunc, Value(static_cast<NodeReal&>(expr).val));
		return realType;
	case NodeType::STR:
		compileConstant(curFunc, Value(new String(static_cast<NodeString&>(expr).val)));
		return stringType;
	case NodeType::SYM: {
		NodeSymbol& expr2 = static_cast<NodeSymbol&>(expr);
		if(expr2.val == "nil") {
			compileConstant(curFunc, Value::nil());
			return nilType;
		} else if(expr2.val == "true") {
			compileConstant(curFunc, Value(true));
			return boolType;
		} else if(expr2.val == "false") {
			compileConstant(curFunc, Value(false));
			return boolType;
		} else {
			throw CompileError("Unexpected keyword in expression: " + expr2.val);
		}
	} case NodeType::ID: {
		NodeId& expr2 = static_cast<NodeId&>(expr);
		std::optional<Variable> var = ctx.getVariable(expr2.val);
		if(var) {
			writeUI8(curFunc.codeOut, (uint8_t) Opcode::LOCAL);
			writeI16(curFunc.codeOut, var->idx);
			return var->type;
		} else {
			auto it = globals->map.find(expr2.val);
			if(it != globals->map.end()) {
				writeUI8(curFunc.codeOut, (uint8_t) Opcode::GLOBAL);
				writeUI16(curFunc.codeOut, curChunk->constants->vec.size());
				curChunk->constants->vec.emplace_back(new String(expr2.val));
				return it->second;
			} else {
				throw CompileError("Trying to access unknown variable: " + expr2.val);
			}
		}
	} case NodeType::UNI_OP: {
		NodeUnary& expr2 = static_cast<NodeUnary&>(expr);
		Type* valType = compileExpression(curFunc, *expr2.val, ctx);
		if(expr2.op == "-") {
			writeUI8(curFunc.codeOut, (uint8_t) Opcode::UNI_MINUS);
			return valType;
		} else if(expr2.op == "not") {
			writeUI8(curFunc.codeOut, (uint8_t) Opcode::NOT);
			return boolType;
		} else {
			throw CompileError("Unknown unary operator: " + expr2.op);
		}
	} case NodeType::BIN_OP: {
		NodeBinary& expr2 = static_cast<NodeBinary&>(expr);
		Type* type1 = compileExpression(curFunc, *expr2.left, ctx);
		Type* type2 = compileExpression(curFunc, *expr2.right, ctx);
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
		bool i1 = type1->canBeAssignedTo(intType),  i2 = type2->canBeAssignedTo(intType),
		     r1 = type1->canBeAssignedTo(realType), r2 = type2->canBeAssignedTo(realType);
		if(numericOps.find(expr2.op) != numericOps.end()) {
			if(i1 && i2) {
				return intType;
			} else if(r1 && r2) {
				return realType;
			} else {
				throw CompileError("Trying to perform arithmetic on " + type1->getDesc() + " and " + type2->getDesc());
			}
		} else if(comparisonOps.find(expr2.op) != comparisonOps.end()) {
			if(r1 && r2) {
				return boolType;
			} else {
				throw CompileError("Trying to compare " + type1->getDesc() + " and " + type2->getDesc());
			}
		} else if(expr2.op == "==" || expr2.op == "!=") {
			return boolType;
		} else if(expr2.op == "/" || expr2.op == "^") {
			if(r1 && r2) {
				return realType;
			} else {
				throw CompileError("Trying to perform real operations on " + type1->getDesc() + " and " + type2->getDesc());
			}
		} else if(expr2.op == "and" || expr2. op == "or") {
			if(type1->canBeAssignedTo(boolType) && type2->canBeAssignedTo(boolType)) {
				return boolType;
			} else {
				throw CompileError("Trying to perform boolean operations on " + type1->getDesc() + " and " + type2->getDesc());
			}
		} else if(expr2.op == "index") {
			ListType* listType = dynamic_cast<ListType*>(type1);
			if(listType && listType->elemType && type2->canBeAssignedTo(intType)) {
				return listType->elemType;
			} else {
				throw CompileError("Trying to index " + type1->getDesc() + " with " + type2->getDesc());
			}
		}
		throw CompileError("Type deduction not implemented for operator " + expr2.op);
	} case NodeType::CALL: {
		NodeCall& expr2 = static_cast<NodeCall&>(expr);
		std::vector<Type*> argTypes;
		for(auto& arg : expr2.args) {
			argTypes.push_back(compileExpression(curFunc, *arg, ctx));
		}
		Type* funcType = compileExpression(curFunc, *expr2.func, ctx);
		writeUI8(curFunc.codeOut, (uint8_t) Opcode::CALL);
		writeUI16(curFunc.codeOut, (uint16_t) expr2.args.size());
		
		FunctionType* funcType2;
		if(funcType->canBeAssignedTo(macroType)) {
			return anyType;
		} else if(funcType2 = dynamic_cast<FunctionType*>(funcType)) {
			uint32_t expected = funcType2->argTypes.size();
			uint32_t got = argTypes.size();
			if(got != expected)
				throw CompileError("Expected " + std::to_string(expected) + " arguments in function call, got " + std::to_string(got));
			for(uint32_t i = 0; i < got; i++) {
				if(!argTypes[i]->canBeAssignedTo(funcType2->argTypes[i]))
					throw CompileError("Cannot assign " + argTypes[i]->getDesc() + " to " + funcType2->argTypes[i]->getDesc() + " argument");
			}
			return funcType2->resType;
		} else {
			throw CompileError("Trying to call " + funcType->getDesc());
		}
	} case NodeType::FUNC: {
		NodeFunction& expr2 = static_cast<NodeFunction&>(expr);
		writeUI8(curFunc.codeOut, (uint8_t) Opcode::MAKE_FUNC);
		if(curChunk->functions.size() > 0xffff)
			throw CompileError("Too many functions in program");
		writeUI16(curFunc.codeOut, (uint16_t) curChunk->functions.size());
		if(expr2.argNames.size() > 0xffff)
			throw CompileError("Too many arguments in function definition");
		writeUI16(curFunc.codeOut, (uint16_t) expr2.argNames.size());
		std::vector<Type*> argTypes;
		for(auto& argTypeDesc : expr2.argTypes) {
			argTypes.push_back(getType(*argTypeDesc));
		}
		Type* resType = nullptr;
		std::vector<int16_t> upvalues = compileFunction(static_cast<NodeBlock&>(*expr2.block), expr2.argNames, argTypes, &resType, &ctx);
		if(upvalues.size() > 0xffff)
			throw CompileError("Too many upvalues in function definition");
		writeUI16(curFunc.codeOut, (uint16_t) upvalues.size());
		for(int16_t upvalue : upvalues) {
			writeI16(curFunc.codeOut, upvalue);
		}
		return new FunctionType(argTypes, resType);
	} case NodeType::LIST: {
		NodeList& expr2 = static_cast<NodeList&>(expr);
		Type* elemType = nullptr;
		for(Node* val : expr2.vals) {
			Type* elemType2 = compileExpression(curFunc, *val, ctx);
			if(elemType) {
				if(elemType->canBeAssignedTo(elemType2)) {
					elemType = elemType2;
				} else if(!elemType2->canBeAssignedTo(elemType)) {
					throw CompileError("Cannot mix " + elemType->getDesc() + " and " + elemType2->getDesc() + " in list literal");
				}
			} else {
				elemType = elemType2;
			}
		}
		writeUI8(curFunc.codeOut, (uint8_t) Opcode::MAKE_LIST);
		if(expr2.vals.size() > 0xffff)
			throw CompileError("Too many elements in list literal");
		writeUI16(curFunc.codeOut, (uint16_t) expr2.vals.size());
		return new ListType(elemType);
	} default:
		throw CompileError("Expression type not implemented: " + nodeTypeDesc(expr.type));
	}
}

void Compiler::compileConstant(FunctionChunk& curFunc, Value val) {
	writeUI8(curFunc.codeOut, (uint8_t) Opcode::CONSTANT);
	writeUI16(curFunc.codeOut, (uint16_t) curChunk->constants->vec.size());
	curChunk->constants->vec.push_back(val);
}
