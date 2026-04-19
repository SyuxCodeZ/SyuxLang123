#pragma once
#include "../ast.h"
#include "token.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

class Parser {
  std::vector<Token> tokens;
  size_t pos = 0;

  const Token& current() const;
  bool isAtEnd() const;
  bool check(TokenType type) const;
  bool match(TokenType type);
  bool isExpressionStart() const;
  const Token& consume(TokenType type, const char* message);
  [[noreturn]] void failHere(const std::string& message) const;

  std::unique_ptr<BlockNode> parseBlock();
  std::unique_ptr<StmtNode> parseStatement();
  std::unique_ptr<StmtNode> parseExpressionStatement();
  std::unique_ptr<StmtNode> parseVarDecl();
  std::unique_ptr<StmtNode> parseAssign();
  std::unique_ptr<StmtNode> parseObjDecl();
  std::unique_ptr<StmtNode> parseOut();
  std::unique_ptr<StmtNode> parseIn();
  std::unique_ptr<StmtNode> parseIf();
  std::unique_ptr<StmtNode> parseWhile();
  std::unique_ptr<StmtNode> parseFor();
  std::unique_ptr<StmtNode> parseForEach();
  std::unique_ptr<StmtNode> parseReturn();
  std::unique_ptr<StmtNode> parseForInit();
  std::unique_ptr<StmtNode> parseForUpdate();
  std::unique_ptr<StmtNode> parseInlineAssign();
  std::unique_ptr<FunctionNode> parseFunction();
  std::unique_ptr<StructNode> parseStruct();
  std::unique_ptr<ClassNode> parseClass();
  std::unique_ptr<ExprNode> parseExpression();
  std::unique_ptr<ExprNode> parseComparison();
  std::unique_ptr<ExprNode> parseAddition();
  std::unique_ptr<ExprNode> parseMultiplication();
  std::unique_ptr<ExprNode> parseAtom();
  std::unique_ptr<ExprNode> parsePrimary();
  std::string inferExprKind(const ExprNode* expr) const;
public:
  Parser(std::vector<Token>);
  std::unique_ptr<ProgramNode> parse();
};