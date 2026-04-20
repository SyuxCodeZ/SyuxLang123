#include "codegen.hpp"
#include <iostream>
#include <typeinfo>
#include <stdexcept>

std::string CodeGen::indent() const {
  return std::string(static_cast<size_t>(indentLevel) * 2, ' ');
}

std::string CodeGen::inferArrayElementType(const ArrayLiteralNode* arr) const {
  if (arr->elements.empty()) return "int";
  const std::string first = inferCppType(arr->elements.front().get());
  if (first == "bool") return "bool";
  if (first == "std::string") return "std::string";
  if (first == "double") return "double";
  return "int";
}

std::string CodeGen::extractVectorElementType(const std::string& vecType) const {
  if (vecType.rfind("std::vector<", 0) == 0) {
    size_t start = 12; // length of "std::vector<"
    size_t end = vecType.rfind(">");
    if (end != std::string::npos && end > start) {
      return vecType.substr(start, end - start);
    }
  }
  return "auto";
}

std::string CodeGen::inferCppType(const ExprNode* expr) const {
  if (const auto* literal = dynamic_cast<const LiteralNode*>(expr)) {
    if (literal->kind == LiteralNode::Kind::Bool) return "bool";
    if (literal->kind == LiteralNode::Kind::String) return "std::string";
    if (literal->kind == LiteralNode::Kind::Float) return "double";
    return "int";
  }
if (const auto* ref = dynamic_cast<const VariableNode*>(expr)) {
    auto it = symbolTable.find(ref->name);
    if (it != symbolTable.end()) {
      if (ref->name == "crypto") return "SyuxCrypto";
      if (ref->name == "data") return "SyuxData";
      if (ref->name == "sys") return "SyuxSys";
      if (ref->name == "thread") return "SyuxThread";
      if (ref->name == "math") return "SyuxMath";
      if (ref->name == "file") return "SyuxFile";
      if (ref->name == "json") return "SyuxJSON";
      return it->second;
    }
    return ref->name;
  }
  if (const auto* call = dynamic_cast<const CallNode*>(expr)) {
    auto it = symbolTable.find(call->callee);
    if (it != symbolTable.end()) return it->second;
    return "auto";
  }
  if (const auto* methodCall = dynamic_cast<const MethodCallNode*>(expr)) {
    return "auto";
  }
  if (const auto* binary = dynamic_cast<const BinaryExprNode*>(expr)) {
    const std::string left = inferCppType(binary->left.get());
    const std::string right = inferCppType(binary->right.get());
    if (left == "std::string" || right == "std::string") return "std::string";
    if (left == "double" || right == "double") return "double";
    if (left == "int" && right == "int") return "int";
  }
  if (const auto* arr = dynamic_cast<const ArrayLiteralNode*>(expr)) {
    return "std::vector<" + inferArrayElementType(arr) + ">";
  }
  if (const auto* idx = dynamic_cast<const IndexExprNode*>(expr)) {
    const std::string collectionType = inferCppType(idx->collection.get());
    if (collectionType.rfind("std::vector<", 0) == 0) {
      return extractVectorElementType(collectionType);
    }
    return collectionType;
  }
  if (dynamic_cast<const PropertyAccessNode*>(expr)) {
    return "auto";
  }
  if (dynamic_cast<const MethodCallNode*>(expr)) {
    return "auto";
  }
  return "auto";
}

std::string CodeGen::emitExpr(const ExprNode* expr) const {
  if (const auto* literal = dynamic_cast<const LiteralNode*>(expr)) {
    if (literal->kind == LiteralNode::Kind::String) {
      std::string escaped;
      for (char c : literal->value) {
        if (c == '"') escaped += "\\\"";
        else if (c == '\\') escaped += "\\\\";
        else if (c == '\n') escaped += "\\n";
        else if (c == '\t') escaped += "\\t";
        else escaped += c;
      }
      return "\"" + escaped + "\"";
    }
    return literal->value;
  }
  if (const auto* ref = dynamic_cast<const VariableNode*>(expr)) {
    return ref->name;
  }
  if (const auto* unary = dynamic_cast<const UnaryMinusNode*>(expr)) {
    return "-" + emitExpr(unary->operand.get());
  }
  if (const auto* call = dynamic_cast<const CallNode*>(expr)) {
    std::string result = call->callee + "(";
    for (size_t i = 0; i < call->args.size(); ++i) {
      if (i) result += ", ";
      result += emitExpr(call->args[i].get());
    }
    result += ")";
    return result;
  }
  if (const auto* methodCall = dynamic_cast<const MethodCallNode*>(expr)) {
    const std::string obj = emitExpr(methodCall->object.get());
    std::string method = methodCall->method;
    std::string objType = inferCppType(methodCall->object.get());
    std::string innerType = objType;
    if (innerType.find("std::vector<") != std::string::npos) {
      size_t start = innerType.find("<") + 1;
      size_t end = innerType.find(">");
      innerType = innerType.substr(start, end - start);
    }
    std::string result;
    if (method == "map" && !methodCall->args.empty()) {
      const auto* lambda = dynamic_cast<const LambdaNode*>(methodCall->args[0].get());
      if (lambda) {
        result = "([&](){ std::vector<" + innerType + "> __r; __r.reserve(" + obj + ".size()); std::transform(" + obj + ".begin(), " + obj + ".end(), std::back_inserter(__r), [&](const " + innerType + "& " + lambda->param + ") { return " + emitExpr(lambda->body.get()) + "; }); return __r; }())";
        return result;
      }
    }
    if (method == "filter" && !methodCall->args.empty()) {
      const auto* lambda = dynamic_cast<const LambdaNode*>(methodCall->args[0].get());
      if (lambda) {
        result = "([&](){ std::vector<" + innerType + "> __filtered; for (const " + innerType + "& " + lambda->param + " : " + obj + ") { if (" + emitExpr(lambda->body.get()) + ") { __filtered.push_back(" + lambda->param + "); } } return __filtered; }())";
        return result;
      }
    }
    if (method == "reduce") {
      result = "([&](){ int __r = 0; for (const auto& __v : " + obj + ") { __r += __v; } return __r; }())";
      return result;
    }
    if (method == "includes") {
      result = "(std::find(" + obj + ".begin(), " + obj + ".end(), ";
      if (!methodCall->args.empty()) {
        result += emitExpr(methodCall->args[0].get());
      } else {
        result += innerType + "()";
      }
      result += ") != " + obj + ".end())";
      return result;
    }
    result = obj + "." + method + "(";
    bool isStaticLib = (obj == "syxcrypto" || obj == "syxdata" || obj == "syxsys" || obj == "syxthread" || obj == "syxmath" || obj == "syxfile" || obj == "syxjson" || obj == "crypto" || obj == "data" || obj == "sys" || obj == "thread" || obj == "math" || obj == "file" || obj == "json" || obj == "SyuxCrypto" || obj == "SyuxData" || obj == "SyuxSys" || obj == "SyuxThread" || obj == "SyuxMath" || obj == "SyuxFile" || obj == "SyuxJSON");
    bool needComma = false;
    if (!isStaticLib) {
      result += obj;
      needComma = true;
    }
    for (size_t i = 0; i < methodCall->args.size(); ++i) {
      if (needComma) result += ", ";
      result += emitExpr(methodCall->args[i].get());
      needComma = true;
    }
    result += ")";
    return result;
  }
  if (const auto* binary = dynamic_cast<const BinaryExprNode*>(expr)) {
    if (binary->op == "+") {
      std::string left = emitExpr(binary->left.get());
      std::string right = emitExpr(binary->right.get());
      const bool leftStr = (left.find("\"") != std::string::npos);
      const bool rightStr = (right.find("\"") != std::string::npos);
      std::string l = leftStr ? "std::string(" + left + ")" : left;
      std::string r = rightStr ? "std::string(" + right + ")" : right;
      return "(" + l + " + " + r + ")";
    }
    return "(" + emitExpr(binary->left.get()) + " " + binary->op + " " + emitExpr(binary->right.get()) + ")";
  }
  if (const auto* arr = dynamic_cast<const ArrayLiteralNode*>(expr)) {
    std::string result = "{";
    for (size_t i = 0; i < arr->elements.size(); ++i) {
      if (i) result += ", ";
      result += emitExpr(arr->elements[i].get());
    }
    result += "}";
    return result;
  }
  if (const auto* idx = dynamic_cast<const IndexExprNode*>(expr)) {
    const std::string collection = emitExpr(idx->collection.get());
    const std::string index = emitExpr(idx->index.get());
    return "([&](){ int __idx = " + index + "; if(__idx >= 0 && __idx < static_cast<int>(" + collection +
      ".size())) return " + collection + "[__idx]; throw std::runtime_error(\"Index out of bounds\"); }())";
  }
  if (const auto* access = dynamic_cast<const PropertyAccessNode*>(expr)) {
    const std::string objCode = emitExpr(access->object.get());
    const std::string prop = access->property;
    
    if (prop == "len") {
      return "static_cast<int>(" + objCode + ".size())";
    }
    
    const std::string objType = inferCppType(access->object.get());
    if (objType == "std::string") {
      if (prop == "length") return objCode + ".length()";
      if (prop == "size") return "static_cast<int>(" + objCode + ".size())";
      if (prop == "trim") return objCode + ".trim()";
      if (prop == "toUpper") return objCode + ".toUpper()";
      if (prop == "toLower") return objCode + ".toLower()";
      if (prop == "split") return "(" + objCode + " + \"\")\"";
    }
    if (objType.find("std::vector") != std::string::npos) {
      if (prop == "len") return "static_cast<int>(" + objCode + ".size())";
      if (prop == "length") return "static_cast<int>(" + objCode + ".size())";
      if (prop == "push") return "(" + objCode + ".push_back(";
      if (prop == "pop") return "(" + objCode + ".pop_back(";
      if (prop == "front") return objCode + ".front()";
      if (prop == "back") return objCode + ".back()";
      if (prop == "empty") return objCode + ".empty()";
      if (prop == "first") return "(" + objCode + ".empty() ? int() : " + objCode + ".front())";
      if (prop == "last") return "(" + objCode + ".empty() ? int() : " + objCode + ".back())";
      if (prop == "sum") return "accumulate(" + objCode + ".begin(), " + objCode + ".end(), int())";
      if (prop == "max") return "(*max_element(" + objCode + ".begin(), " + objCode + ".end()))";
      if (prop == "min") return "(*min_element(" + objCode + ".begin(), " + objCode + ".end()))";
      if (prop == "avg") return "([&](){ double s = 0; for (const auto& __v : " + objCode + ") s += __v; return " + objCode + ".empty() ? 0 : s / " + objCode + ".size(); }())";
      if (prop == "sort") return "([&](){ auto __v = " + objCode + "; sort(__v.begin(), __v.end()); return __v; }())";
      if (prop == "join") return "([&](){ string __s; for (size_t __i = 0; __i < " + objCode + ".size(); ++__i) { __s += to_string(" + objCode + "[__i]); if (__i + 1 < " + objCode + ".size()) __s += \",\"; } return __s; }())";
    }
    
    return objCode + "." + prop;
  }
  throw std::runtime_error("[Syux Error] Unknown expression node in codegen.");
}

std::string CodeGen::emitInlineStatement(const StmtNode* stmt) const {
  if (const auto* var = dynamic_cast<const VarDeclNode*>(stmt)) {
    return inferCppType(var->value.get()) + " " + var->name + " = " + emitExpr(var->value.get());
  }
  if (const auto* assign = dynamic_cast<const AssignmentNode*>(stmt)) {
    return assign->name + " = " + emitExpr(assign->value.get());
  }
  if (const auto* inc = dynamic_cast<const IncrementNode*>(stmt)) {
    return inc->name + "++";
  }
  if (const auto* dec = dynamic_cast<const DecrementNode*>(stmt)) {
    return dec->name + "--";
  }
  throw std::runtime_error("[Syux Error] Invalid statement in inline context (for loop init/update).");
}

void CodeGen::emitStatement(const StmtNode* stmt) {
  if (const auto* var = dynamic_cast<const VarDeclNode*>(stmt)) {
    const std::string type = inferCppType(var->value.get());
    symbolTable[var->name] = type;
    out << indent() << type << " " << var->name << " = " << emitExpr(var->value.get()) << ";\n";
    return;
  }
  if (const auto* assign = dynamic_cast<const AssignmentNode*>(stmt)) {
    auto it = symbolTable.find(assign->name);
    if (it == symbolTable.end()) {
      throw std::runtime_error("[Syux Error] Undefined variable '" + assign->name + "'");
    }
    const std::string varType = it->second;
    const std::string exprType = inferCppType(assign->value.get());
    if (varType != exprType && varType != "auto" && exprType != "auto") {
      bool mismatch = false;
      if ((varType == "int" || varType == "double") && (exprType == "std::string" || exprType == "bool")) mismatch = true;
      if (varType == "std::string" && (exprType == "int" || exprType == "double" || exprType == "bool")) mismatch = true;
      if (varType == "bool" && exprType != "bool") mismatch = true;
      if (mismatch) {
        throw std::runtime_error("[Syux Error] Type mismatch in assignment to '" + assign->name +
                                 "': expected " + varType + ", got " + exprType);
      }
    }
    out << indent() << assign->name << " = " << emitExpr(assign->value.get()) << ";\n";
    return;
  }
  if (const auto* assign = dynamic_cast<const IndexAssignmentNode*>(stmt)) {
    out << indent() << "do {\n";
    ++indentLevel;
    out << indent() << "int __idx = " << emitExpr(assign->index.get()) << ";\n";
    out << indent() << "if(__idx < 0 || __idx >= static_cast<int>(" << assign->name << ".size())) throw std::runtime_error(\"Index out of bounds\");\n";
    out << indent() << assign->name << "[__idx] = " << emitExpr(assign->value.get()) << ";\n";
    --indentLevel;
    out << indent() << "} while(false);\n";
    return;
  }
  if (const auto* propAssign = dynamic_cast<const PropertyAssignmentNode*>(stmt)) {
    out << indent() << emitExpr(propAssign->object.get()) << "." << propAssign->property << " = " << emitExpr(propAssign->value.get()) << ";\n";
    return;
  }
  if (const auto* inc = dynamic_cast<const IncrementNode*>(stmt)) {
    out << indent() << inc->name << "++;\n";
    return;
  }
  if (const auto* dec = dynamic_cast<const DecrementNode*>(stmt)) {
    out << indent() << dec->name << "--;\n";
    return;
  }
  if (const auto* output = dynamic_cast<const OutputNode*>(stmt)) {
    out << indent() << "std::cout << " << emitExpr(output->value.get()) << " << std::endl;\n";
    return;
  }
  if (const auto* input = dynamic_cast<const InputNode*>(stmt)) {
    auto it = symbolTable.find(input->name);
    if (it == symbolTable.end()) {
      symbolTable[input->name] = "std::string";
      out << indent() << "std::string " << input->name << ";\n";
      out << indent() << "std::getline(std::cin, " << input->name << ");\n";
    } else if (it->second == "std::string") {
      out << indent() << "std::getline(std::cin, " << input->name << ");\n";
    } else if (it->second == "int" || it->second == "double") {
      out << indent() << "std::cin >> " << input->name << ";\n";
      out << indent() << "std::cin.ignore();\n";
    } else if (it->second == "bool") {
      out << indent() << "{\n";
      ++indentLevel;
      out << indent() << "std::string __boolInput;\n";
      out << indent() << "std::getline(std::cin, __boolInput);\n";
      out << indent() << input->name << " = (__boolInput == \"true\" || __boolInput == \"on\" || __boolInput == \"1\");\n";
      --indentLevel;
      out << indent() << "}\n";
    } else {
      throw std::runtime_error("[Syux Error] Cannot read input into variable '" + input->name + "' of type " + it->second);
    }
    return;
  }
  if (const auto* obj = dynamic_cast<const ObjectDeclNode*>(stmt)) {
    symbolTable[obj->varName] = obj->typeName;
    if (obj->args.empty()) {
      out << indent() << obj->typeName << " " << obj->varName << "{};\n";
      return;
    }
    out << indent() << obj->typeName << " " << obj->varName << "(";
    for (size_t i = 0; i < obj->args.size(); ++i) {
      if (i) out << ", ";
      out << emitExpr(obj->args[i].get());
    }
    out << ");\n";
    return;
  }
  if (const auto* ifNode = dynamic_cast<const IfNode*>(stmt)) {
    out << indent() << "if (" << emitExpr(ifNode->condition.get()) << ") {\n";
    ++indentLevel;
    emitBlock(ifNode->thenBlock.get());
    --indentLevel;
    if (ifNode->elseBlock) {
      out << indent() << "}";
      auto elseStmt = ifNode->elseBlock->statements[0].get();
      if (dynamic_cast<const IfNode*>(elseStmt)) {
        out << " else ";
        emitStatement(elseStmt);
      } else {
        out << " else {\n";
        ++indentLevel;
        emitBlock(ifNode->elseBlock.get());
        --indentLevel;
        out << indent() << "}\n";
      }
    } else {
      out << indent() << "}\n";
    }
    return;
  }
  if (const auto* switchNode = dynamic_cast<const SwitchNode*>(stmt)) {
    for (size_t i = 0; i < switchNode->caseValues.size(); ++i) {
      if (i == 0) out << indent() << "if (";
      else out << indent() << "else if (";
      out << emitExpr(switchNode->expr.get()) << " == " << emitExpr(switchNode->caseValues[i].get()) << ") {\n";
      ++indentLevel;
      emitBlock(switchNode->caseBlocks[i].get());
      --indentLevel;
      out << indent() << "}\n";
    }
    if (switchNode->defaultBlock) {
      out << indent() << "else {\n";
      ++indentLevel;
      emitBlock(switchNode->defaultBlock.get());
      --indentLevel;
      out << indent() << "}\n";
    }
    return;
  }
  if (const auto* whileNode = dynamic_cast<const WhileNode*>(stmt)) {
    out << indent() << "while (" << emitExpr(whileNode->condition.get()) << ") {\n";
    ++indentLevel;
    emitBlock(whileNode->body.get());
    --indentLevel;
    out << indent() << "}\n";
    return;
  }
  if (const auto* forNode = dynamic_cast<const ForNode*>(stmt)) {
    out << indent() << "for (" << emitInlineStatement(forNode->init.get()) << "; "
      << emitExpr(forNode->condition.get()) << "; " << emitInlineStatement(forNode->update.get()) << ") {\n";
    ++indentLevel;
    emitBlock(forNode->body.get());
    --indentLevel;
    out << indent() << "}\n";
    return;
  }
  if (const auto* each = dynamic_cast<const ForEachNode*>(stmt)) {
    const std::string iterableType = inferCppType(each->iterable.get());
    const std::string elemType = extractVectorElementType(iterableType);
    out << indent() << "for (" << elemType << " " << each->varName << " : " << emitExpr(each->iterable.get()) << ") {\n";
    ++indentLevel;
    symbolTable[each->varName] = elemType;
    emitBlock(each->body.get());
    --indentLevel;
    out << indent() << "}\n";
    return;
  }
  if (const auto* ret = dynamic_cast<const ReturnNode*>(stmt)) {
    out << indent() << "return " << emitExpr(ret->value.get()) << ";\n";
    return;
  }
  if (const auto* methodCall = dynamic_cast<const MethodCallStmtNode*>(stmt)) {
    out << indent() << emitExpr(methodCall->call.get()) << ";\n";
    return;
  }
  if (const auto* call = dynamic_cast<const CallStmtNode*>(stmt)) {
    std::string result = call->callee + "(";
    for (size_t i = 0; i < call->args.size(); ++i) {
      if (i) result += ", ";
      result += emitExpr(call->args[i].get());
    }
    result += ");";
    out << indent() << result << "\n";
    return;
  }
  if (const auto* exprStmt = dynamic_cast<const ExpressionStmtNode*>(stmt)) {
    out << indent() << emitExpr(exprStmt->expr.get()) << ";\n";
    return;
  }
  throw std::runtime_error("[Syux Error] Unknown statement node in codegen.");
}

void CodeGen::emitBlock(const BlockNode* block) {
  const size_t oldSize = symbolTable.size();
  for (const auto& stmt : block->statements) {
    emitStatement(stmt.get());
  }
  auto it = symbolTable.begin();
  std::advance(it, oldSize);
  symbolTable.erase(it, symbolTable.end());
}

void CodeGen::emitFunction(const FunctionNode* fn) {
  out << "auto " << fn->name << "(";
  for (size_t i = 0; i < fn->params.size(); ++i) {
    if (i) out << ", ";
    out << "auto " << fn->params[i];
  }
  out << ") {\n";
  ++indentLevel;
  for (const auto& param : fn->params) {
    symbolTable[param] = "auto";
  }
  emitBlock(fn->body.get());
  --indentLevel;
  out << "}\n\n";
}

void CodeGen::emitStruct(const StructNode* node) {
  out << "struct " << node->name << " {\n";
  out << "public:\n";
  ++indentLevel;
  for (const auto& field : node->fields) {
    const std::string type = inferCppType(field->value.get());
    symbolTable[field->name] = type;
    out << indent() << type << " " << field->name << " = " << emitExpr(field->value.get()) << ";\n";
  }
  out << indent() << node->name << "() = default;\n";
  --indentLevel;
  out << "};\n\n";
}

void CodeGen::emitClass(const ClassNode* node) {
  out << "class " << node->name << " {\n";
  out << "public:\n";
  ++indentLevel;
  
  for (const auto& field : node->fields) {
    const std::string type = inferCppType(field->value.get());
    symbolTable[field->name] = type;
    out << indent() << type << " " << field->name << ";\n";
  }

  if (node->ctor) {
    out << indent() << node->name << "(";
    for (size_t i = 0; i < node->ctor->params.size(); ++i) {
      if (i) out << ", ";
      out << "auto " << node->ctor->params[i];
    }
    out << ") {\n";
    ++indentLevel;
    out << indent() << node->name << "& self = *this;\n";
    for (const auto& field : node->fields) {
      out << indent() << "self." << field->name << " = ";
      if (field->value) {
        out << emitExpr(field->value.get()) << ";\n";
      } else {
        out << "0;\n";
      }
    }
    for (const auto& param : node->ctor->params) {
      symbolTable[param] = "auto";
    }
    symbolTable["self"] = node->name + "&";
    emitBlock(node->ctor->body.get());
    --indentLevel;
    out << indent() << "}\n";
  } else {
    out << indent() << node->name << "() = default;\n";
  }

  if (node->dtor) {
    out << indent() << "~" << node->name << "() {\n";
    ++indentLevel;
    emitBlock(node->dtor->body.get());
    --indentLevel;
    out << indent() << "}\n";
  }

  for (const auto& method : node->methods) {
    out << indent() << "auto " << method->name << "(";
    out << node->name << "& self";
    for (size_t i = 0; i < method->params.size(); ++i) {
      out << ", ";
      out << "auto " << method->params[i];
    }
    out << ") {\n";
    ++indentLevel;
    symbolTable["self"] = node->name + "&";
    for (const auto& param : method->params) {
      symbolTable[param] = "auto";
    }
    emitBlock(method->body.get());
    --indentLevel;
    out << indent() << "}\n";
  }
  --indentLevel;
  out << "};\n\n";
}

void CodeGen::generate(const ProgramNode* program){
  out.str("");
  out.clear();
  symbolTable.clear();
  indentLevel = 0;

  bool needsIO = false, needsString = false, needsVector = false, needsMath = false, needsFile = false, needsJSON = false, needsHTTP = false, needsRand = false, needsTime = false, needsRegex = false, needsGame = false, needsGUI = false, needsData = false, needsCrypto = false, needsSys = false, needsThread = false;
  
  for (const auto& imp : program->imports) {
    if (imp == "io" || imp == "file") { needsIO = true; needsFile = true; }
    if (imp == "str" || imp == "string") needsString = true;
    if (imp == "vec" || imp == "vector") needsVector = true;
    if (imp == "math" || imp == "Math") needsMath = true;
    if (imp == "json" || imp == "JSON") { needsJSON = true; needsIO = true; }
    if (imp == "http" || imp == "api") { needsHTTP = true; needsIO = true; }
    if (imp == "type" || imp == "types") needsType = true;
    if (imp == "rand" || imp == "random") needsRand = true;
    if (imp == "time" || imp == "Time") needsTime = true;
    if (imp == "regex" || imp == "Regex") needsRegex = true;
    if (imp == "game" || imp == "Game") { needsGame = true; needsIO = true; }
    if (imp == "gui" || imp == "GUI") { needsGUI = true; needsIO = true; }
    if (imp == "data" || imp == "Data") needsData = true;
    if (imp == "crypto" || imp == "Crypto") needsCrypto = true;
    if (imp == "sys" || imp == "Sys" || imp == "system") needsSys = true;
    if (imp == "thread" || imp == "Thread") needsThread = true;
    if (imp == "stl" || imp == "STL") { needsIO = true; needsString = true; needsVector = true; needsMath = true; }
  }

  out << "#include <iostream>\n#include <vector>\n#include <algorithm>\n#include <numeric>\n";
  if (needsString || needsMath || needsVector) out << "#include <string>\n";
  if (needsMath) out << "#include <cmath>\n";
  if (needsString) out << "#include <algorithm>\n";
  if (needsFile) out << "#include <fstream>\n";
  if (needsJSON) out << "#include <variant>\n";
  if (needsHTTP) out << "#include <windows.h>\n#include <winhttp.h>\n#pragma comment(lib, \"winhttp.lib\")\n";
  if (needsRand) out << "#include <random>\n";
  if (needsTime) out << "#include <chrono>\n#include <thread>\n";
  if (needsRegex) out << "#include <regex>\n";
  if (needsData) out << "#include <sstream>\n#include <iomanip>\n#include <map>\n#include <fstream>\n#include <cmath>\n";
  if (needsCrypto) out << "#include <openssl/sha.h>\n#include <openssl/md5.h>\n";
  if (needsSys) out << "#include <map>\n";
  if (needsSys) {
    out << "#ifdef _WIN32\n";
    out << "#include <windows.h>\n#include <processthreadsapi.h>\n";
    out << "#include <psapi.h>\n#pragma comment(lib, \"psapi\")\n";
    out << "#else\n";
    out << "#include <unistd.h>\n#include <sys/stat.h>\n#include <sys/sysinfo.h>\n";
    out << "#endif\n";
  }
  if (needsThread) out << "#include <thread>\n#include <mutex>\n#include <atomic>\n#include <future>\n";
  if (needsGame) {
    out << "#ifdef _WIN32\n";
    out << "#include <windows.h>\n#include <chrono>\n";
    out << "#elif __APPLE__\n";
    out << "#include <SDL2/SDL.h>\n#pragma comment(lib, \"SDL2\")\n";
    out << "#else\n";
    out << "#include <X11/Xlib.h>\n#include <X11/keysym.h>\n#pragma comment(lib, \"X11\")\n";
    out << "#endif\n";
  }
  if (needsGUI) {
    out << "#ifdef _WIN32\n";
    out << "#include <windows.h>\n";
    out << "#elif __APPLE__\n";
    out << "#include <SDL2/SDL.h>\n#pragma comment(lib, \"SDL2\")\n";
    out << "#else\n";
    out << "#include <X11/Xlib.h>\n#pragma comment(lib, \"X11\")\n";
    out << "#endif\n";
  }
  out << "#include <stdexcept>\n\n";

  out << "using namespace std;\n";
  if (needsFile) {
    out << "struct SyuxFile {\n";
    out << "  static string read(const string& path) {\n";
    out << "    ifstream f(path); string r, line;\n";
    out << "    while (getline(f, line)) r += line + \"\\n\";\n";
    out << "    f.close(); return r;\n";
    out << "  }\n";
    out << "  static void write(const string& path, const string& content) {\n";
    out << "    ofstream f(path); f << content; f.close();\n";
    out << "  }\n";
    out << "  static void append(const string& path, const string& content) {\n";
    out << "    ofstream f(path, ios::app); f << content; f.close();\n";
    out << "  }\n";
    out << "};\n";
    out << "SyuxFile file;\n\n";
    symbolTable["file"] = "SyuxFile";
  }
  if (needsJSON) {
    out << "struct SyuxJSON {\n";
    out << "  static string stringify(const string& s) { return s; }\n";
    out << "  static string parse(const string& s) { return s; }\n";
    out << "};\n";
    out << "SyuxJSON json;\n\n";
    symbolTable["json"] = "SyuxJSON";
  }
  if (needsHTTP) {
    out << "#ifdef _WIN32\n";
    out << "#include <windows.h>\n#include <winhttp.h>\n#pragma comment(lib, \"winhttp.lib\")\n";
    out << "#else\n";
    out << "#include <sys/socket.h>\n#include <netinet/in.h>\n#include <arpa/inet.h>\n#include <netdb.h>\n#include <unistd.h>\n#include <fcntl.h>\n";
    out << "#endif\n";
    out << "struct SyuxHTTP {\n";
    out << "  static string get(const string& url) {\n";
    out << "#ifdef _WIN32\n";
    out << "    URL_COMPONENTS uc; ZeroMemory(&uc, sizeof(uc));\n";
    out << "    uc.dwStructSize = sizeof(uc); wchar_t host[256], path[1024];\n";
    out << "    uc.lpszHostName = host; uc.lpszUrlPath = path; uc.dwUrlPathLength = 1024;\n";
    out << "    std::wstring wurl(url.begin(), url.end());\n";
    out << "    if (!WinHttpCrackUrl(wurl.c_str(), (DWORD)wurl.size(), 0, &uc)) return \"\";\n";
    out << "    HINTERNET hSession = WinHttpOpen(L\"Syux/1.0\", 0, NULL, NULL, 0);\n";
    out << "    HINTERNET hConnect = WinHttpConnect(hSession, host, uc.nPort, 0);\n";
    out << "    if (!hConnect) { WinHttpCloseHandle(hSession); return \"\"; }\n";
    out << "    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L\"GET\", path, NULL, NULL, NULL, 0);\n";
    out << "    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return \"\"; }\n";
    out << "    WinHttpSendRequest(hRequest, NULL, 0, NULL, 0, 0, 0);\n";
    out << "    WinHttpReceiveResponse(hRequest, NULL);\n";
    out << "    std::string result; DWORD dwSize = 0;\n";
    out << "    do { WinHttpQueryDataAvailable(hRequest, &dwSize);\n";
    out << "      if (dwSize == 0) break;\n";
    out << "      char* buffer = new char[dwSize + 1]; DWORD dwRead = 0;\n";
    out << "      WinHttpReadData(hRequest, buffer, dwSize, &dwRead);\n";
    out << "      buffer[dwRead] = 0; result += buffer; delete[] buffer; } while (dwSize > 0);\n";
    out << "    WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);\n";
    out << "    return result;\n";
    out << "#else\n";
    out << "    size_t pos = url.find(\"://\"); size_t p2 = url.find(\"/\", pos+3);\n";
    out << "    std::string host = (pos==string::npos)?url.substr(7):url.substr(pos+3, p2-pos-3);\n";
    out << "    std::string path = (p2==string::npos)?\"/\":url.substr(p2);\n";
    out << "    struct hostent* he = gethostbyname(host.c_str());\n";
    out << "    if (!he) return \"\"; int sock = socket(AF_INET, SOCK_STREAM, 0);\n";
    out << "    if (sock < 0) return \"\"; struct sockaddr_in sa; sa.sin_family = AF_INET;\n";
    out << "    memcpy(&sa.sin_addr, he->h_addr, he->h_length); sa.sin_port = htons(80);\n";
    out << "    if (connect(sock, (struct sockaddr*)&sa, sizeof(sa)) { close(sock); return \"\"; }\n";
    out << "    std::string req = \"GET \" + path + \" HTTP/1.1\\r\\nHost: \" + host + \"\\r\\nConnection: close\\r\\n\\r\\n\";\n";
    out << "    send(sock, req.c_str(), req.size(), 0); std::string r; char buf[4096];\n";
    out << "    ssize_t n; while ((n=recv(sock, buf, sizeof(buf)-1, 0)) > 0) { buf[n]=0; r+=buf; }\n";
    out << "    close(sock); size_t b = r.find(\"\\r\\n\\r\\n\"); return (b==string::npos)?r:r.substr(b+4);\n";
    out << "#endif\n";
    out << "  }\n";
    out << "  static string post(const string& url, const string& data) {\n";
    out << "#ifdef _WIN32\n";
    out << "    return get(url); // basic fallback\n";
    out << "#else\n";
    out << "    size_t pos = url.find(\"://\"); size_t p2 = url.find(\"/\", pos+3);\n";
    out << "    std::string host = (pos==string::npos)?url.substr(7):url.substr(pos+3, p2-pos-3);\n";
    out << "    std::string path = (p2==string::npos)?\"/\":url.substr(p2);\n";
    out << "    struct hostent* he = gethostbyname(host.c_str());\n";
    out << "    if (!he) return \"\"; int sock = socket(AF_INET, SOCK_STREAM, 0);\n";
    out << "    if (sock < 0) return \"\"; struct sockaddr_in sa; sa.sin_family = AF_INET;\n";
    out << "    memcpy(&sa.sin_addr, he->h_addr, he->h_length); sa.sin_port = htons(80);\n";
    out << "    if (connect(sock, (struct sockaddr*)&sa, sizeof(sa))) { close(sock); return \"\"; }\n";
    out << "    std::string req = \"POST \" + path + \" HTTP/1.1\\r\\nHost: \" + host + \"\\r\\nContent-Type: application/json\\r\\nContent-Length: \" + std::to_string(data.size()) + \"\\r\\n\\r\\n\" + data;\n";
    out << "    send(sock, req.c_str(), req.size(), 0); std::string r; char buf[4096]; ssize_t n;\n";
    out << "    while ((n=recv(sock, buf, sizeof(buf)-1, 0)) > 0) { buf[n]=0; r+=buf; }\n";
    out << "    close(sock); size_t b = r.find(\"\\r\\n\\r\\n\"); return (b==string::npos)?r:r.substr(b+4);\n";
    out << "#endif\n";
    out << "  }\n";
    out << "};\n";
    out << "SyuxHTTP httpapi;\n\n";
    symbolTable["http"] = "SyuxHTTP";
  }
  if (needsType) {
    out << "#include <typeinfo>\n";
    out << "struct SyuxType {\n";
    out << "  static string typeOf(int) { return \"int\"; }\n";
    out << "  static string typeOf(double) { return \"double\"; }\n";
    out << "  static string typeOf(const std::string&) { return \"string\"; }\n";
    out << "  static string typeOf(bool) { return \"bool\"; }\n";
    out << "  static string typeOf(const char*) { return \"string\"; }\n";
    out << "  static string typeOf(...) { return \"unknown\"; }\n";
    out << "  static bool isNumber(int) { return true; }\n";
    out << "  static bool isNumber(double) { return true; }\n";
    out << "  static bool isNumber(...) { return false; }\n";
    out << "  static bool isString(const std::string&) { return true; }\n";
    out << "  static bool isString(const char*) { return true; }\n";
    out << "  static bool isString(...) { return false; }\n";
    out << "  static bool isBool(bool) { return true; }\n";
    out << "  static bool isBool(...) { return false; }\n";
    out << "};\n";
    out << "SyuxType type;\n\n";
    symbolTable["type"] = "SyuxType";
  }
  out << "\n";

  if (needsMath) {
    out << "struct SyuxMath {\n";
    out << "  static double abs(double x) { return x < 0 ? -x : x; }\n";
    out << "  static double round(double x) { return floor(x + 0.5); }\n";
    out << "  static double random() { return (double)rand() / RAND_MAX; }\n";
    out << "  static double max(double a, double b) { return a > b ? a : b; }\n";
    out << "  static double min(double a, double b) { return a < b ? a : b; }\n";
    out << "  static double pow(double base, double exp) { return std::pow(base, exp); }\n";
    out << "  static double sqrt(double x) { return std::sqrt(x); }\n";
    out << "  static double floor(double x) { return std::floor(x); }\n";
    out << "  static double ceil(double x) { return std::ceil(x); }\n";
    out << "  static double sin(double x) { return std::sin(x); }\n";
    out << "  static double cos(double x) { return std::cos(x); }\n";
    out << "  static double tan(double x) { return std::tan(x); }\n";
    out << "  static double log(double x) { return std::log(x); }\n";
    out << "  static double log10(double x) { return std::log10(x); }\n";
    out << "  static double exp(double x) { return std::exp(x); }\n";
    out << "  static double atan(double x) { return std::atan(x); }\n";
    out << "  static double atan2(double y, double x) { return std::atan2(y, x); }\n";
    out << "  static double asin(double x) { return std::asin(x); }\n";
    out << "  static double acos(double x) { return std::acos(x); }\n";
    out << "  static double sinh(double x) { return std::sinh(x); }\n";
    out << "  static double cosh(double x) { return std::cosh(x); }\n";
    out << "  static double tanh(double x) { return std::tanh(x); }\n";
    out << "  static double hypot(double x, double y) { return std::hypot(x, y); }\n";
    out << "  static double cbrt(double x) { return std::cbrt(x); }\n";
    out << "  static double erf(double x) { return std::erf(x); }\n";
    out << "  static double erfc(double x) { return std::erfc(x); }\n";
    out << "  static double tgamma(double x) { return std::tgamma(x); }\n";
    out << "  static double lgamma(double x) { return std::lgamma(x); }\n";
    out << "  static double trunc(double x) { return std::trunc(x); }\n";
    out << "  static double fmod(double a, double b) { return std::fmod(a, b); }\n";
    out << "  static double remainder(double a, double b) { return std::remainder(a, b); }\n";
    out << "  static int roundi(double x) { return (int)std::round(x); }\n";
    out << "  static long long roundl(double x) { return (long long)std::round(x); }\n";
    out << "};\n";
    out << "SyuxMath math;\n\n";
    symbolTable["math"] = "SyuxMath";
  }

  if (needsRand) {
    out << "#include <random>\n";
    out << "struct SyuxRand {\n";
    out << "  static std::mt19937 __rng;\n";
    out << "  static int intRange(int min, int max) { std::uniform_int_distribution<int> d(min, max); return d(__rng); }\n";
    out << "  static double doubleRange(double min, double max) { std::uniform_real_distribution<double> d(min, max); return d(__rng); }\n";
    out << "  static int intn(int n) { std::uniform_int_distribution<int> d(0, n-1); return d(__rng); }\n";
    out << "  static void shuffle(vector<int>& v) { std::shuffle(v.begin(), v.end(), __rng); }\n";
    out << "  static void seed(int s) { __rng.seed(s); }\n";
    out << "};\n";
    out << "std::mt19937 SyuxRand::__rng(std::random_device{}());\n";
    out << "SyuxRand syxrand;\n\n";
    symbolTable["rand"] = "SyuxRand";
  }

  if (needsTime) {
    out << "#include <chrono>\n#include <thread>\n";
    out << "struct SyuxClock {\n";
    out << "  static long long now() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(); }\n";
    out << "  static long long nowMicros() { return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count(); }\n";
    out << "  static void sleep(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }\n";
    out << "  static void sleepSeconds(int s) { std::this_thread::sleep_for(std::chrono::seconds(s)); }\n";
    out << "};\n";
    out << "SyuxClock syxtime;\n\n";
    symbolTable["time"] = "SyuxClock";
  }

  if (needsRegex) {
    out << "#include <regex>\n";
    out << "struct SyuxRegex {\n";
    out << "  static bool matches(const string& s, const string& pattern) { return std::regex_match(s, std::regex(pattern)); }\n";
    out << "  static string replace(const string& s, const string& pattern, const string& repl) { return std::regex_replace(s, std::regex(pattern), repl); }\n";
    out << "  static bool search(const string& s, const string& pattern) { return std::regex_search(s, std::regex(pattern)); }\n";
    out << "};\n";
    out << "SyuxRegex syxregex;\n\n";
    symbolTable["regex"] = "SyuxRegex";
  }

  if (needsGame) {
    out << "struct SyuxGame {\n";
    out << "#ifdef _WIN32\n";
    out << "  static HWND __hwnd; static HDC __hdc;\n";
    out << "  static void init(const string& title, int w, int h) {\n";
    out << "    __width = w; __height = h; __running = true;\n";
    out << "    WNDCLASSW wc = {0}; wc.lpfnWndProc = [](HWND h, UINT m, WPARAM wp, LPARAM lp) { return DefWindowProcW(h, m, wp, lp); };\n";
    out << "    wc.hInstance = GetModuleHandleW(NULL); RegisterClassW(&wc);\n";
    out << "    std::wstring wt(title.begin(), title.end());\n";
    out << "    __hwnd = CreateWindowW(L\"STATIC\", wt.c_str(), WS_OVERLAPPEDWINDOW, 100, 100, w, h, NULL, NULL, wc.hInstance, NULL);\n";
    out << "    ShowWindow(__hwnd, SW_SHOW); __hdc = GetDC(__hwnd);\n";
    out << "  }\n";
    out << "  static void clear(int r, int g, int b) { HBRUSH br = CreateSolidBrush(RGB(r,g,b)); SelectObject(__hdc, br); Rectangle(__hdc, 0, 0, __width+1, __height+1); }\n";
    out << "  static void drawRect(int x, int y, int w, int h, int r, int g, int b) { HBRUSH br = CreateSolidBrush(RGB(r,g,b)); SelectObject(__hdc, br); Rectangle(__hdc, x, y, x+w, y+h); }\n";
    out << "  static void drawCircle(int x, int y, int rad, int r, int g, int b) { HBRUSH br = CreateSolidBrush(RGB(r,g,b)); SelectObject(__hdc, br); Ellipse(__hdc, x-rad, y-rad, x+rad, y+rad); }\n";
    out << "  static void drawLine(int x1, int y1, int x2, int y2, int r, int g, int b) { HPEN pen = CreatePen(PS_SOLID, 1, RGB(r,g,b)); SelectObject(__hdc, pen); MoveToEx(__hdc, x1, y1, NULL); LineTo(__hdc, x2, y2); }\n";
    out << "  static void drawText(int x, int y, const string& s, int sz, int r, int g, int b) { std::wstring ws(s.begin(), s.end()); SetTextColor(__hdc, RGB(r,g,b)); SetBkMode(__hdc, TRANSPARENT); HFONT fnt = CreateFontW(sz, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, L\"Arial\"); SelectObject(__hdc, fnt); TextOutW(__hdc, x, y, ws.c_str(), (int)ws.size()); }\n";
    out << "  static bool keyDown(int vk) { return GetAsyncKeyState(vk) < 0; }\n";
    out << "  static int mouseX() { POINT pt; GetCursorPos(&pt); ScreenToClient(__hwnd, &pt); return pt.x; }\n";
    out << "  static int mouseY() { POINT pt; GetCursorPos(&pt); ScreenToClient(__hwnd, &pt); return pt.y; }\n";
    out << "  static bool mouseDown(int btn) { return GetAsyncKeyState(btn) < 0; }\n";
    out << "#elif __APPLE__\n";
    out << "  static SDL_Window* __win; static SDL_Renderer* __ren;\n";
    out << "  static void init(const string& title, int w, int h) {\n";
    out << "    __width = w; __height = h; __running = true;\n";
    out << "    SDL_Init(SDL_INIT_VIDEO); __win = SDL_CreateWindow(title.c_str(), 100, 100, w, h, SDL_WINDOW_SHOWN); __ren = SDL_CreateRenderer(__win, -1, 0);\n";
    out << "  }\n";
    out << "  static void clear(int r, int g, int b) { SDL_SetRenderDrawColor(__ren, r, g, b, 255); SDL_RenderClear(__ren); }\n";
    out << "  static void drawRect(int x, int y, int w, int h, int r, int g, int b) { SDL_SetRenderDrawColor(__ren, r, g, b, 255); SDL_Rect rct = {x, y, w, h}; SDL_RenderFillRect(__ren, &rct); }\n";
    out << "  static void drawCircle(int x, int y, int rad, int r, int g, int b) { SDL_SetRenderDrawColor(__ren, r, g, b, 255); for (int i = 0; i < 360; i += 5) { float a = i * 3.14159 / 180; SDL_RenderDrawPoint(__ren, x + (int)(rad * cosf(a)), y + (int)(rad * sinf(a))); } }\n";
    out << "  static void drawLine(int x1, int y1, int x2, int y2, int r, int g, int b) { SDL_SetRenderDrawColor(__ren, r, g, b, 255); SDL_RenderDrawLine(__ren, x1, y1, x2, y2); }\n";
    out << "  static void drawText(int x, int y, const string& s, int sz, int r, int g, int b) { }\n";
    out << "  static bool keyDown(int vk) { SDL_PumpEvents(); const Uint8* k = SDL_GetKeyboardState(NULL); return k[vk]; }\n";
    out << "  static int mouseX() { int x, y; SDL_GetMouseState(&x, &y); return x; }\n";
    out << "  static int mouseY() { int x, y; SDL_GetMouseState(&x, &y); return y; }\n";
    out << "  static bool mouseDown(int btn) { return SDL_GetMouseState(NULL, NULL) & btn; }\n";
    out << "#else\n";
    out << "  static Display* __dpy; static Window __win;\n";
    out << "  static void init(const string& title, int w, int h) {\n";
    out << "    __width = w; __height = h; __running = true;\n";
    out << "    __dpy = XOpenDisplay(NULL); int scr = DefaultScreen(__dpy);\n";
    out << "    __win = XCreateSimpleWindow(__dpy, RootWindow(__dpy, scr), 100, 100, w, h, 0, 0, BlackPixel(__dpy, scr));\n";
    out << "    std::wstring wt(title.begin(), title.end());	XStoreName(__dpy, __win, (char*)title.c_str());\n";
    out << "    XSelectInput(__dpy, __win, ExposureMask|KeyPressMask|ButtonPressMask|ButtonReleaseMask);\n";
    out << "    XMapWindow(__dpy, __win);\n";
    out << "  }\n";
    out << "  static void clear(int r, int g, int b) { XSetForeground(__dpy, DefaultGC(__dpy, 0), (r<<16)|(g<<8)|b; XFillRectangle(__dpy, __win, DefaultGC(__dpy, 0), 0, 0, __width, __height); }\n";
    out << "  static void drawRect(int x, int y, int w, int h, int r, int g, int b) { XSetForeground(__dpy, DefaultGC(__dpy, 0), (r<<16)|(g<<8)|b); XFillRectangle(__dpy, __win, DefaultGC(__dpy, 0), x, y, w, h); }\n";
    out << "  static void drawCircle(int x, int y, int rad, int r, int g, int b) { XSetForeground(__dpy, DefaultGC(__dpy, 0), (r<<16)|(g<<8)|b); XDrawArc(__dpy, __win, DefaultGC(__dpy, 0), x-rad, y-rad, rad*2, rad*2, 0, 360*64); }\n";
    out << "  static void drawLine(int x1, int y1, int x2, int y2, int r, int g, int b) { XSetForeground(__dpy, DefaultGC(__dpy, 0), (r<<16)|(g<<8)|b); XDrawLine(__dpy, __win, DefaultGC(__dpy, 0), x1, y1, x2, y2); }\n";
    out << "  static void drawText(int x, int y, const string& s, int sz, int r, int g, int b) { XSetForeground(__dpy, DefaultGC(__dpy, 0), (r<<16)|(g<<8)|b); XDrawString(__dpy, __win, DefaultGC(__dpy, 0), x, y, s.c_str(), (int)s.size()); }\n";
    out << "  static bool keyDown(int vk) { char keys[32]; XQueryKeymap(__dpy, keys); return keys[vk/8] & (1 << (vk%8)); }\n";
    out << "  static int mouseX() { Window r; int x, y; unsigned int m; XQueryPointer(__dpy, __win, &r, &r, &x, &y, &m, &m); return x; }\n";
    out << "  static int mouseY() { Window r; int x, y; unsigned int m; XQueryPointer(__dpy, __win, &r, &r, &x, &y, &m, &m); return y; }\n";
    out << "  static bool mouseDown(int btn) { Window r; int x, y; unsigned int m; XQueryPointer(__dpy, __win, &r, &r, &x, &y, &m, &m); return m & btn; }\n";
    out << "#endif\n";
    out << "  static int __width, __height;\n";
    out << "  static bool __running;\n";
    out << "  static std::vector<void(*)()> __updateHandlers;\n";
    out << "  static std::vector<void(*)()> __drawHandlers;\n";
    out << "  static void onUpdate(void(*fn)()) { __updateHandlers.push_back(fn); }\n";
    out << "  static void onDraw(void(*fn)()) { __drawHandlers.push_back(fn); }\n";
    out << "  static void run() {\n";
    out << "#ifdef _WIN32\n";
    out << "    MSG m; while (__running && PeekMessageW(&m, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&m); DispatchMessageW(&m); for (auto fn : __updateHandlers) fn(); for (auto fn : __drawHandlers) { SelectObject(__hdc, GetStockObject(WHITE_BRUSH)); Rectangle(__hdc, 0, 0, __width, __height); fn(); } Sleep(16); }\n";
    out << "#elif __APPLE__\n";
    out << "    SDL_Event e; while (__running && SDL_PollEvent(&e)) { if (e.type == SDL_QUIT) __running = false; } for (auto fn : __updateHandlers) fn(); for (auto fn : __drawHandlers) fn(); SDL_RenderPresent(__ren); SDL_Delay(16);\n";
    out << "#else\n";
    out << "    while (__running) { XEvent e; while (XPending(__dpy)) { XNextEvent(__dpy, &e); if (e.type == Expose) __running = false; } for (auto fn : __updateHandlers) fn(); for (auto fn : __drawHandlers) fn(); usleep(16000); }\n";
    out << "#endif\n";
    out << "  }\n";
    out << "  static bool keyPressed(int vk) { static int __p[256] = {0}; bool k = keyDown(vk); bool r = k && !__p[vk]; __p[vk] = k; return r; }\n";
    out << "  static void exit() { __running = false; }\n";
    out << "};\n";
    out << "SyuxGame game;\n\n";
    symbolTable["game"] = "SyuxGame";
  }

  if (needsGUI) {
    out << "struct SyuxGUI {\n";
    out << "#ifdef _WIN32\n";
    out << "  static HWND __hwnd;\n";
    out << "  static int __nextId;\n";
    out << "  static std::vector<HWND> __buttons;\n";
    out << "  static std::vector<void(*)()> __buttonCallbacks;\n";
    out << "  static void init(const string& title, int w, int h) {\n";
    out << "    __nextId = 1; __hwnd = NULL;\n";
    out << "    WNDCLASSW wc = {0}; wc.lpfnWndProc = [](HWND h, UINT m, WPARAM wp, LPARAM lp) -> LRESULT { if (m == WM_COMMAND && HIWORD(wp) == BN_CLICKED) { int id = LOWORD(wp); if (id >= 1 && id <= (int)SyuxGUI::__buttonCallbacks.size()) SyuxGUI::__buttonCallbacks[id-1](); } return DefWindowProcW(h, m, wp, lp); };\n";
    out << "    wc.hInstance = GetModuleHandleW(NULL); RegisterClassW(&wc);\n";
    out << "    std::wstring wt(title.begin(), title.end());\n";
    out << "    __hwnd = CreateWindowW(L\"STATIC\", wt.c_str(), WS_OVERLAPPEDWINDOW, 100, 100, w, h, NULL, NULL, wc.hInstance, NULL);\n";
    out << "    ShowWindow(__hwnd, SW_SHOW);\n";
    out << "  }\n";
    out << "  static int button(int x, int y, int w, int h, const string& label) { std::wstring wl(label.begin(), label.end()); int id = __nextId++; HWND bh = CreateWindowW(L\"BUTTON\", wl.c_str(), WS_TABSTOP|WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON, x, y, w, h, __hwnd, (HMENU)id, GetModuleHandleW(NULL), NULL); __buttons.push_back(bh); __buttonCallbacks.push_back([](){}); return id; }\n";
    out << "  static void label(int x, int y, const string& text) { std::wstring ws(text.begin(), text.end()); CreateWindowW(L\"STATIC\", ws.c_str(), WS_VISIBLE|WS_CHILD, x, y, 200, 25, __hwnd, NULL, GetModuleHandleW(NULL), NULL); }\n";
    out << "  static string input(int x, int y, int w, int h) { int id = __nextId++; HWND ih = CreateWindowW(L\"EDIT\", L\"\", WS_VISIBLE|WS_CHILD|WS_BORDER|ES_AUTOHSCROLL, x, y, w, h, __hwnd, (HMENU)id, GetModuleHandleW(NULL), NULL); char buf[256] = {0}; GetWindowTextA(ih, buf, 255); return std::string(buf); }\n";
    out << "  static void setText(int id, const string& text) { if (id > 0 && id <= (int)__buttons.size()) { std::wstring ws(text.begin(), text.end()); SetWindowTextW(__buttons[id-1], ws.c_str()); } }\n";
    out << "  static void onClick(int id, void(*fn)()) { if (id > 0 && id <= (int)__buttonCallbacks.size()) __buttonCallbacks[id-1] = fn; }\n";
    out << "  static void show() { MSG m; while (GetMessageW(&m, NULL, 0, 0)) { TranslateMessage(&m); DispatchMessageW(&m); } }\n";
    out << "#elif __APPLE__\n";
    out << "  static SDL_Window* __win; static SDL_Renderer* __ren;\n";
    out << "  static void init(const string& title, int w, int h) { SDL_Init(SDL_INIT_VIDEO); __win = SDL_CreateWindow(title.c_str(), 100, 100, w, h, SDL_WINDOW_SHOWN); __ren = SDL_CreateRenderer(__win, -1, 0); }\n";
    out << "  static void label(int x, int y, const string& text) { }\n";
    out << "  static int button(int x, int y, int w, int h, const string& label) { return 0; }\n";
    out << "  static string input(int x, int y, int w, int h) { return \"\"; }\n";
    out << "  static void setText(int id, const string& text) { }\n";
    out << "  static void onClick(int id, void(*fn)()) { }\n";
    out << "  static void show() { SDL_Event e; while (1) { while (SDL_PollEvent(&e)) { if (e.type == SDL_QUIT) return; } SDL_Delay(16); } }\n";
    out << "#else\n";
    out << "  static Display* __dpy; static Window __win;\n";
    out << "  static void init(const string& title, int w, int h) { __dpy = XOpenDisplay(NULL); int scr = DefaultScreen(__dpy); __win = XCreateSimpleWindow(__dpy, RootWindow(__dpy, scr), 100, 100, w, h, 0, 0, BlackPixel(__dpy, scr)); XStoreName(__dpy, __win, (char*)title.c_str()); XSelectInput(__dpy, __win, ExposureMask|KeyPressMask); XMapWindow(__dpy, __win); }\n";
    out << "  static void label(int x, int y, const string& text) { XDrawString(__dpy, __win, DefaultGC(__dpy, 0), x, y, text.c_str(), (int)text.size()); }\n";
    out << "  static int button(int x, int y, int w, int h, const string& label) { return 0; }\n";
    out << "  static string input(int x, int y, int w, int h) { return \"\"; }\n";
    out << "  static void setText(int id, const string& text) { }\n";
    out << "  static void onClick(int id, void(*fn)()) { }\n";
    out << "  static void show() { XEvent e; while (1) { XNextEvent(__dpy, &e); if (e.type == Expose) XFlush(__dpy); } }\n";
    out << "#endif\n";
    out << "};\n";
    out << "SyuxGUI gui;\n\n";
    symbolTable["gui"] = "SyuxGUI";
  }

  if (needsData) {
    out << "struct SyuxData {\n";
    out << "  static vector<vector<string>> fromCSV(const string& path) {\n";
    out << "    vector<vector<string>> rows; ifstream f(path); string line;\n";
    out << "    while (getline(f, line)) {\n";
    out << "      vector<string> row; stringstream ss(line); string cell;\n";
    out << "      while (getline(ss, cell, ',')) row.push_back(cell);\n";
    out << "      rows.push_back(row);\n";
    out << "    }\n";
    out << "    return rows;\n";
    out << "  }\n";
    out << "  static double median(vector<int> v) { sort(v.begin(), v.end()); int n = v.size(); if (n%2==0) return (v[n/2-1]+v[n/2])/2.0; return v[n/2]; }\n";
    out << "  static double variance(vector<int> v) { double m = 0; for (int x : v) m += x; m /= v.size(); double s = 0; for (int x : v) s += (x-m)*(x-m); return s/v.size(); }\n";
    out << "  static double stdDev(vector<int> v) { return sqrt(variance(v)); }\n";
    out << "  static int mode(vector<int> v) { std::map<int,int> c; for (int x : v) c[x]++; int mx = 0, mo = v[0]; for (auto& p : c) if (p.second > mx) { mx = p.second; mo = p.first; } return mo; }\n";
    out << "};\n";
    out << "SyuxData syxdata;\n\n";
    symbolTable["data"] = "SyuxData";
  }

  if (needsCrypto) {
    out << "struct SyuxCrypto {\n";
    out << "  static string md5(const string& s) { unsigned char d[16]; MD5((unsigned char*)s.c_str(), s.size(), d); char r[33]; for (int i=0;i<16;i++) sprintf(r+i*2, \"%02x\", d[i]); return string(r, 32); }\n";
    out << "  static string sha256(const string& s) { unsigned char d[32]; SHA256((unsigned char*)s.c_str(), s.size(), d); char r[65]; for (int i=0;i<32;i++) sprintf(r+i*2, \"%02x\", d[i]); return string(r, 64); }\n";
    out << "  static string base64Encode(const string& s) { static const char* b = \"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/\"; int i = 0; char r[((s.size()+2)/3)*4]; for (size_t p=0;p<s.size();p+=3) { int q = (s[p]<<16)+(s[p+1]<<8)+s[p+2]; r[i++] = b[(q>>18)&63]; r[i++] = b[(q>>12)&63]; r[i++] = s[p+1] ? b[(q>>6)&63] : '='; r[i++] = s[p+2] ? b[q&63] : '='; } r[i] = 0; return string(r); }\n";
    out << "  static string xorEncrypt(const string& s, const string& k) { string r; for (size_t i=0;i<s.size();i++) r += (char)(s[i] ^ k[i % k.size()]); return r; }\n";
    out << "};\n";
    out << "SyuxCrypto syxcrypto;\n\n";
    symbolTable["crypto"] = "SyuxCrypto";
  }

  if (needsSys) {
    out << "struct SyuxSys {\n";
    out << "#ifdef _WIN32\n";
    out << "  static int exec(const string& cmd) { return system(cmd.c_str()); }\n";
    out << "  static string getEnv(const string& name) { char* v = getenv(name.c_str()); return v ? v : \"\"; }\n";
    out << "  static void setEnv(const string& name, const string& value) { _putenv_s(name.c_str(), value.c_str()); }\n";
    out << "  static string osInfo() { OSVERSIONINFO os = {sizeof(OSVERSIONINFO)}; GetVersionEx(&os); return \"Windows \" + to_string(os.dwMajorVersion) + \".\" + to_string(os.dwMinorVersion); }\n";
    out << "  static int cpuCores() { SYSTEM_INFO si; GetSystemInfo(&si); return si.dwNumberOfProcessors; }\n";
    out << "  static long long memAvailable() { MEMORYSTATUSEX ms; ms.dwLength = sizeof(ms); GlobalMemoryStatusEx(&ms); return ms.ullAvailPhys; }\n";
    out << "  static void clipboard(const string& s) { if (OpenClipboard(NULL)) { EmptyClipboard(); HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, s.size()+1); char* p = (char*)GlobalLock(h); strcpy(p, s.c_str()); GlobalUnlock(h); SetClipboardData(CF_TEXT, h); CloseClipboard(); } }\n";
    out << "  static string clipboard() { if (!OpenClipboard(NULL)) return \"\"; HGLOBAL h = GetClipboardData(CF_TEXT); char* p = (char*)GlobalLock(h); string r(p); GlobalUnlock(h); CloseClipboard(); return r; }\n";
    out << "#else\n";
    out << "  static int exec(const string& cmd) { return system(cmd.c_str()); }\n";
    out << "  static string getEnv(const string& name) { char* v = getenv(name.c_str()); return v ? v : \"\"; }\n";
    out << "  static void setEnv(const string& name, const string& value) { setenv(name.c_str(), value.c_str(), 1); }\n";
    out << "  static string osInfo() { struct sysinfo si; sysinfo(&si); return \"Linux \" + to_string(si.sysversion); }\n";
    out << "  static int cpuCores() { return get_nprocs(); }\n";
    out << "  static long long memAvailable() { struct sysinfo si; sysinfo(&si); return si.freeram * si.mem_unit; }\n";
    out << "  static void clipboard(const string& s) { }\n";
    out << "  static string clipboard() { return \"\"; }\n";
    out << "#endif\n";
    out << "};\n";
    out << "SyuxSys syxsys;\n\n";
    symbolTable["sys"] = "SyuxSys";
  }

  if (needsThread) {
    out << "struct SyuxThread {\n";
    out << "  static void run(void(*fn)()) { thread t(fn); t.detach(); }\n";
    out << "  static void sleep(int ms) { this_thread::sleep_for(chrono::milliseconds(ms)); }\n";
    out << "  template<typename T> static T atomicGet(atomic<T>& a) { return a.load(); }\n";
    out << "  template<typename T> static void atomicSet(atomic<T>& a, T v) { a.store(v); }\n";
    out << "};\n";
    out << "SyuxThread syxthread;\n\n";
    symbolTable["thread"] = "SyuxThread";
  }

  if (needsString) {
    out << "struct SyuxString {\n";
    out << "  static string trim(const string& s) {\n";
    out << "    size_t start = s.find_first_not_of(\" \\t\\n\\r\");\n";
    out << "    if (start == string::npos) return \"\";\n";
    out << "    size_t end = s.find_last_not_of(\" \\t\\n\\r\");\n";
    out << "    return s.substr(start, end - start + 1);\n";
    out << "  }\n";
    out << "  static string toUpper(const string& s) {\n";
    out << "    string r = s; transform(r.begin(), r.end(), r.begin(), ::toupper); return r;\n";
    out << "  }\n";
    out << "  static string toLower(const string& s) {\n";
    out << "    string r = s; transform(r.begin(), r.end(), r.begin(), ::tolower); return r;\n";
    out << "  }\n";
    out << "  static vector<string> split(const string& s, const string& delim) {\n";
    out << "    vector<string> r; size_t start = 0;\n";
    out << "    while (true) {\n";
    out << "      size_t pos = s.find(delim, start);\n";
out << "    if (pos == string::npos) { r.push_back(s.substr(start)); break; }\n";
    out << "      r.push_back(s.substr(start, pos - start));\n";
    out << "      start = pos + delim.size();\n";
    out << "    }\n";
    out << "    return r;\n";
    out << "  }\n";
    out << "  static string join(const vector<string>& v, const string& delim) {\n";
    out << "    string r; for (size_t i = 0; i < v.size(); ++i) {\n";
    out << "      r += v[i]; if (i < v.size() - 1) r += delim;\n";
    out << "    }\n";
    out << "    return r;\n";
    out << "  }\n";
    out << "  static string replace(const string& s, const string& from, const string& to) {\n";
    out << "    string r = s; size_t pos = 0;\n";
    out << "    while ((pos = r.find(from, pos)) != string::npos) {\n";
    out << "      r.replace(pos, from.size(), to);\n";
    out << "      pos += to.size();\n";
    out << "    }\n";
    out << "    return r;\n";
    out << "  }\n";
    out << "  static int indexOf(const string& s, const string& sub) {\n";
    out << "    return (int)s.find(sub);\n";
    out << "  }\n";
    out << "  static bool contains(const string& s, const string& sub) {\n";
    out << "    return s.find(sub) != string::npos;\n";
    out << "  }\n";
    out << "  static string substring(const string& s, int start, int end) {\n";
    out << "    return s.substr(start, end - start);\n";
    out << "  }\n";
    out << "  static bool startsWith(const string& s, const string& prefix) {\n";
    out << "    return s.rfind(prefix, 0) == 0;\n";
    out << "  }\n";
    out << "  static bool endsWith(const string& s, const string& suffix) {\n";
    out << "    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;\n";
    out << "  }\n";
    out << "};\n\n";
    
    out << "struct console {\n";
    out << "  template<typename T> static void log(T v) { cout << v << endl; }\n";
    out << "  template<typename T> static void error(T v) { cerr << v << endl; }\n";
    out << "  template<typename T> static void warn(T v) { cerr << \"WARNING: \" << v << endl; }\n";
    out << "  template<typename T> static void info(T v) { cout << \"INFO: \" << v << endl; }\n";
    out << "};\n\n";
  }

  for (const auto& node : program->structs) emitStruct(node.get());
  for (const auto& node : program->classes) emitClass(node.get());
  for (const auto& fn : program->functions) emitFunction(fn.get());

  for (const auto& stmt : program->topLevelVars) {
    emitStatement(stmt.get());
  }

  out << "int main() {";
  ++indentLevel;
  out << indent() << "std::cout.setf(std::ios::boolalpha);\n";
  if (program->mainBlock) {
    emitBlock(program->mainBlock.get());
  }
  out << indent() << "return 0;\n";
  --indentLevel;
  out << "}\n";
}

std::string CodeGen::str() const { return out.str(); }