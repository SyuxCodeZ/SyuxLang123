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
    if (it != symbolTable.end()) return it->second;
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
    if (literal->kind == LiteralNode::Kind::String) return "\"" + literal->value + "\"";
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
    std::string result = obj + "." + methodCall->method + "(" + obj;
    for (size_t i = 0; i < methodCall->args.size(); ++i) {
      result += ", ";
      result += emitExpr(methodCall->args[i].get());
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
    out << indent() << "}";
    if (ifNode->elseBlock) {
      out << " else {\n";
      ++indentLevel;
      emitBlock(ifNode->elseBlock.get());
      --indentLevel;
      out << indent() << "}\n";
    } else {
      out << "\n";
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
    out << indent() << type << " " << field->name << " = " << emitExpr(field->value.get()) << ";\n";
  }

  if (node->ctor) {
    out << indent() << node->name << "(";
    for (size_t i = 0; i < node->ctor->params.size(); ++i) {
      if (i) out << ", ";
      out << "auto " << node->ctor->params[i];
    }
    out << ") {\n";
    ++indentLevel;
    for (const auto& param : node->ctor->params) {
      symbolTable[param] = "auto";
    }
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

  bool needsIO = false, needsString = false, needsVector = false, needsMath = false, needsFile = false, needsJSON = false;
  
  for (const auto& imp : program->imports) {
    if (imp == "io" || imp == "file") { needsIO = true; needsFile = true; }
    if (imp == "str" || imp == "string") needsString = true;
    if (imp == "vec" || imp == "vector") needsVector = true;
    if (imp == "math" || imp == "Math") needsMath = true;
    if (imp == "json" || imp == "JSON") { needsJSON = true; needsIO = true; }
    if (imp == "stl" || imp == "STL") { needsIO = true; needsString = true; needsVector = true; needsMath = true; }
  }

  out << "#include <iostream>\n";
  if (needsString || needsMath || needsVector) out << "#include <string>\n";
  if (needsVector) out << "#include <vector>\n#include <algorithm>\n";
  if (needsMath) out << "#include <cmath>\n";
  if (needsString) out << "#include <algorithm>\n";
  if (needsFile) out << "#include <fstream>\n";
  if (needsJSON) out << "#include <nlohmann/json.hpp>\n";
  out << "#include <stdexcept>\n\n";

  out << "using namespace std;\n";
  if (needsJSON) out << "using json = nlohmann::json;\n";
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
    out << "};\n\n";
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