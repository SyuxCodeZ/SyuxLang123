#pragma once
#include "../ast.h"
#include <iostream>
#include <sstream>
#include <unordered_map>

class CodeGen {
  std::ostringstream out;
  std::unordered_map<std::string, std::string> symbolTable;
  int indentLevel = 0;

  std::string indent() const;
  std::string inferArrayElementType(const ArrayLiteralNode* arr) const;
  std::string extractVectorElementType(const std::string& vecType) const;
  std::string inferCppType(const ExprNode* expr) const;
  std::string emitExpr(const ExprNode* expr) const;
  std::string emitInlineStatement(const StmtNode* stmt) const;
  void emitStatement(const StmtNode* stmt);
  void emitBlock(const BlockNode* block);
  void emitFunction(const FunctionNode* fn);
  void emitStruct(const StructNode* node);
  void emitClass(const ClassNode* node);
public:
  void generate(const ProgramNode* program);
  std::string str() const;
};