#include "parser.hpp"
#include <iostream>
#include <stdexcept>

Parser::Parser(std::vector<Token> t):tokens(t){}

[[noreturn]] void Parser::failHere(const std::string& message) const {
  const int line = current().line;
  throw std::runtime_error("[Syux Error] Line " + std::to_string(line) + ": " + message);
}

const Token& Parser::current() const {
  if (pos >= tokens.size()) {
    return tokens.back();
  }
  return tokens[pos];
}

bool Parser::isAtEnd() const {
  return current().type == TokenType::End;
}

bool Parser::check(TokenType type) const {
  if (isAtEnd()) return type == TokenType::End;
  return current().type == type;
}

bool Parser::match(TokenType type) {
  if (check(type)) {
    ++pos;
    return true;
  }
  return false;
}

bool Parser::isExpressionStart() const {
  return check(TokenType::Number) || check(TokenType::Float) || check(TokenType::String) ||
    check(TokenType::Identifier) || check(TokenType::LParen) ||
    check(TokenType::True) || check(TokenType::False) ||
    check(TokenType::On) || check(TokenType::Off) ||
    check(TokenType::LBracket) || check(TokenType::Minus);
}

const Token& Parser::consume(TokenType type, const char* message) {
  if (!check(type)) {
    failHere(message);
  }
  return tokens[pos++];
}

std::unique_ptr<BlockNode> Parser::parseBlock() {
  consume(TokenType::LBracket, "Expected '[' to start block.");
  auto block = std::make_unique<BlockNode>();

  while (!check(TokenType::RBracket) && !isAtEnd()) {
    block->statements.push_back(parseStatement());
  }

  consume(TokenType::RBracket, "Expected ']' to close block.");
  return block;
}

std::unique_ptr<StmtNode> Parser::parseStatement() {
  if (check(TokenType::Val)) return parseVarDecl();
  if (check(TokenType::Set)) return parseAssign();
  if (check(TokenType::Obj)) return parseObjDecl();
  if (check(TokenType::CompOut)) return parseOut();
  if (check(TokenType::CompIn)) return parseIn();
  if (check(TokenType::If)) return parseIf();
  if (check(TokenType::While)) return parseWhile();
  if (check(TokenType::For)) return parseFor();
  if (check(TokenType::Return)) return parseReturn();
  if (check(TokenType::Identifier)) return parseExpressionStatement();
  failHere("Expected statement (val, set, obj, comp.out, comp.in, if, while, for, return).");
}

std::unique_ptr<StmtNode> Parser::parseExpressionStatement() {
  auto expr = parseExpression();
  auto stmt = std::make_unique<ExpressionStmtNode>();
  stmt->expr = std::move(expr);
  return stmt;
}

std::unique_ptr<StmtNode> Parser::parseVarDecl() {
  consume(TokenType::Val, "Expected 'val'.");
  const Token& name = consume(TokenType::Identifier, "Expected variable name.");
  consume(TokenType::Equal, "Expected '=' in variable declaration.");
  if (!isExpressionStart()) {
    failHere("Expected expression after '='.");
  }

  auto stmt = std::make_unique<VarDeclNode>();
  stmt->name = name.text;
  stmt->value = parseExpression();
  return stmt;
}

std::unique_ptr<StmtNode> Parser::parseAssign() {
  consume(TokenType::Set, "Expected 'set'.");
  const Token& name = consume(TokenType::Identifier, "Expected variable name.");
  if (match(TokenType::LBracket)) {
    auto index = parseExpression();
    consume(TokenType::RBracket, "Expected ']' after index.");
    consume(TokenType::Equal, "Expected '=' in assignment.");
    if (!isExpressionStart()) {
      failHere("Expected expression after '='.");
    }
    auto stmt = std::make_unique<IndexAssignmentNode>();
    stmt->name = name.text;
    stmt->index = std::move(index);
    stmt->value = parseExpression();
    return stmt;
  }
  if (match(TokenType::Dot)) {
    const Token& prop = consume(TokenType::Identifier, "Expected property name after '.'.");
    consume(TokenType::Equal, "Expected '=' in property assignment.");
    if (!isExpressionStart()) {
      failHere("Expected expression after '='.");
    }
    auto stmt = std::make_unique<PropertyAssignmentNode>();
    auto var = std::make_unique<VariableNode>();
    var->name = name.text;
    stmt->object = std::move(var);
    stmt->property = prop.text;
    stmt->value = parseExpression();
    return stmt;
  }
  consume(TokenType::Equal, "Expected '=' in assignment.");
  if (!isExpressionStart()) {
    failHere("Expected expression after '='.");
  }

  auto stmt = std::make_unique<AssignmentNode>();
  stmt->name = name.text;
  stmt->value = parseExpression();
  return stmt;
}

std::unique_ptr<StmtNode> Parser::parseObjDecl() {
  consume(TokenType::Obj, "Expected 'obj'.");
  const Token& typeName = consume(TokenType::Identifier, "Expected type name.");
  const Token& varName = consume(TokenType::Identifier, "Expected variable name.");
  consume(TokenType::LParen, "Expected '(' after object variable name.");

  auto stmt = std::make_unique<ObjectDeclNode>();
  stmt->typeName = typeName.text;
  stmt->varName = varName.text;

  if (!check(TokenType::RParen)) {
    stmt->args.push_back(parseExpression());
    while (match(TokenType::Comma)) {
      stmt->args.push_back(parseExpression());
    }
  }

  consume(TokenType::RParen, "Expected ')' after object arguments.");
  return stmt;
}

std::unique_ptr<StmtNode> Parser::parseOut() {
  consume(TokenType::CompOut, "Expected 'comp.out'.");

  auto stmt = std::make_unique<OutputNode>();
  stmt->value = parseExpression();
  return stmt;
}

std::unique_ptr<StmtNode> Parser::parseIn() {
  consume(TokenType::CompIn, "Expected 'comp.in'.");
  const Token& name = consume(TokenType::Identifier, "Expected variable name.");
  auto stmt = std::make_unique<InputNode>();
  stmt->name = name.text;
  return stmt;
}

std::unique_ptr<StmtNode> Parser::parseIf() {
  consume(TokenType::If, "Expected 'if'.");
  consume(TokenType::LParen, "Expected '(' after if.");
  auto condition = parseExpression();
  consume(TokenType::RParen, "Expected ')' after if condition.");
  auto thenBlock = parseBlock();

  std::unique_ptr<BlockNode> elseBlock;
  if (match(TokenType::Else)) {
    elseBlock = parseBlock();
  }

  auto stmt = std::make_unique<IfNode>();
  stmt->condition = std::move(condition);
  stmt->thenBlock = std::move(thenBlock);
  stmt->elseBlock = std::move(elseBlock);
  return stmt;
}

std::unique_ptr<StmtNode> Parser::parseWhile() {
  consume(TokenType::While, "Expected 'while'.");
  consume(TokenType::LParen, "Expected '(' after while.");
  auto condition = parseExpression();
  consume(TokenType::RParen, "Expected ')' after while condition.");
  auto body = parseBlock();

  auto stmt = std::make_unique<WhileNode>();
  stmt->condition = std::move(condition);
  stmt->body = std::move(body);
  return stmt;
}

std::unique_ptr<StmtNode> Parser::parseInlineAssign() {
  const Token& name = consume(TokenType::Identifier, "Expected variable name.");
  consume(TokenType::Equal, "Expected '=' in assignment.");
  if (!isExpressionStart()) {
    failHere("Expected expression after '='.");
  }
  auto stmt = std::make_unique<AssignmentNode>();
  stmt->name = name.text;
  stmt->value = parseExpression();
  return stmt;
}

std::unique_ptr<StmtNode> Parser::parseForInit() {
  if (check(TokenType::Val)) return parseVarDecl();
  if (check(TokenType::Identifier)) return parseInlineAssign();
  failHere("Expected for initializer (val or assignment).");
}

std::unique_ptr<StmtNode> Parser::parseForUpdate() {
  const Token& name = consume(TokenType::Identifier, "Expected loop variable in update.");
  if (match(TokenType::PlusPlus)) {
    auto inc = std::make_unique<IncrementNode>();
    inc->name = name.text;
    return inc;
  }
  if (match(TokenType::MinusMinus)) {
    auto dec = std::make_unique<DecrementNode>();
    dec->name = name.text;
    return dec;
  }
  if (match(TokenType::Equal)) {
    if (!isExpressionStart()) {
      failHere("Expected expression after '=' in for update.");
    }
    auto assign = std::make_unique<AssignmentNode>();
    assign->name = name.text;
    assign->value = parseExpression();
    return assign;
  }
  failHere("Expected '++', '--', or '=' in for update.");
}

std::unique_ptr<StmtNode> Parser::parseFor() {
  consume(TokenType::For, "Expected 'for'.");
  if (check(TokenType::Val)) {
    return parseForEach();
  }
  consume(TokenType::LBrace, "Expected '{' after for.");
  auto init = parseForInit();
  consume(TokenType::Colon, "Expected ':' after for initializer.");
  auto condition = parseExpression();
  consume(TokenType::Colon, "Expected ':' after for condition.");
  auto update = parseForUpdate();
  consume(TokenType::RBrace, "Expected '}' after for header.");
  auto body = parseBlock();

  auto stmt = std::make_unique<ForNode>();
  stmt->init = std::move(init);
  stmt->condition = std::move(condition);
  stmt->update = std::move(update);
  stmt->body = std::move(body);
  return stmt;
}

std::unique_ptr<StmtNode> Parser::parseForEach() {
  consume(TokenType::Val, "Expected 'val' in for-each.");
  const Token& var = consume(TokenType::Identifier, "Expected loop variable name.");
  consume(TokenType::InKw, "Expected 'in' in for-each.");
  // Parse iterable as an atom so the following block '[' is not consumed as indexing.
  auto iterable = parseAtom();
  auto body = parseBlock();

  auto stmt = std::make_unique<ForEachNode>();
  stmt->varName = var.text;
  stmt->iterable = std::move(iterable);
  stmt->body = std::move(body);
  return stmt;
}

std::unique_ptr<StmtNode> Parser::parseReturn() {
  consume(TokenType::Return, "Expected 'return'.");
  if (isExpressionStart()) {
    auto stmt = std::make_unique<ReturnNode>();
    stmt->value = parseExpression();
    return stmt;
  }
  auto stmt = std::make_unique<ReturnNode>();
  auto literal = std::make_unique<LiteralNode>();
  literal->kind = LiteralNode::Kind::Int;
  literal->value = "0";
  stmt->value = std::move(literal);
  return stmt;
}

std::unique_ptr<FunctionNode> Parser::parseFunction() {
  consume(TokenType::Func, "Expected 'func'.");
  auto fn = std::make_unique<FunctionNode>();
  fn->name = consume(TokenType::Identifier, "Expected function name.").text;
  consume(TokenType::LParen, "Expected '(' after function name.");
  if (!check(TokenType::RParen)) {
    fn->params.push_back(consume(TokenType::Identifier, "Expected parameter name.").text);
    while (match(TokenType::Comma)) {
      fn->params.push_back(consume(TokenType::Identifier, "Expected parameter name.").text);
    }
  }
  consume(TokenType::RParen, "Expected ')' after parameters.");
  fn->body = parseBlock();
  return fn;
}

std::unique_ptr<StructNode> Parser::parseStruct() {
  consume(TokenType::Struct, "Expected 'struct'.");
  auto type = std::make_unique<StructNode>();
  type->name = consume(TokenType::Identifier, "Expected struct name.").text;
  consume(TokenType::LBracket, "Expected '[' after struct name.");

  while (!check(TokenType::RBracket) && !isAtEnd()) {
    if (!check(TokenType::Val)) {
      failHere("Expected struct member (val).");
    }
    consume(TokenType::Val, "Expected 'val'.");
    const Token& fieldName = consume(TokenType::Identifier, "Expected field name.");
    auto field = std::make_unique<VarDeclNode>();
    field->name = fieldName.text;
    if (match(TokenType::Equal)) {
      field->value = parseExpression();
    }
    type->fields.push_back(std::move(field));
  }

  consume(TokenType::RBracket, "Expected ']' to close struct declaration.");
  return type;
}

std::unique_ptr<ClassNode> Parser::parseClass() {
  consume(TokenType::Class, "Expected 'class'.");
  auto type = std::make_unique<ClassNode>();
  type->name = consume(TokenType::Identifier, "Expected class name.").text;
  consume(TokenType::LBracket, "Expected '[' after class name.");

  while (!check(TokenType::RBracket) && !isAtEnd()) {
    if (check(TokenType::Val)) {
      consume(TokenType::Val, "Expected 'val'.");
      const Token& fieldName = consume(TokenType::Identifier, "Expected field name.");
      auto field = std::make_unique<VarDeclNode>();
      field->name = fieldName.text;
      if (match(TokenType::Equal)) {
        field->value = parseExpression();
      } else {
        auto expr = std::make_unique<LiteralNode>();
        expr->kind = LiteralNode::Kind::Int;
        expr->value = "0";
        field->value = std::move(expr);
      }
      type->fields.push_back(std::move(field));
      continue;
    }

    if (check(TokenType::Ctor)) {
      consume(TokenType::Ctor, "Expected 'ctor'.");
      auto ctor = std::make_unique<ClassNode::CtorNode>();
      consume(TokenType::LParen, "Expected '(' after ctor.");
      if (!check(TokenType::RParen)) {
        ctor->params.push_back(consume(TokenType::Identifier, "Expected ctor parameter.").text);
        while (match(TokenType::Comma)) {
          ctor->params.push_back(consume(TokenType::Identifier, "Expected ctor parameter.").text);
        }
      }
      consume(TokenType::RParen, "Expected ')' after ctor params.");
      ctor->body = parseBlock();
      type->ctor = std::move(ctor);
      continue;
    }

    if (check(TokenType::Dtor)) {
      consume(TokenType::Dtor, "Expected 'dtor'.");
      auto dtor = std::make_unique<ClassNode::DtorNode>();
      consume(TokenType::LParen, "Expected '(' after dtor.");
      consume(TokenType::RParen, "Expected ')' after dtor.");
      dtor->body = parseBlock();
      type->dtor = std::move(dtor);
      continue;
    }

    if (check(TokenType::Func)) {
      consume(TokenType::Func, "Expected 'func'.");
      auto method = std::make_unique<ClassNode::MethodNode>();
      method->name = consume(TokenType::Identifier, "Expected method name.").text;
      consume(TokenType::LParen, "Expected '(' after method name.");
      if (!check(TokenType::RParen)) {
        method->params.push_back(consume(TokenType::Identifier, "Expected parameter name.").text);
        while (match(TokenType::Comma)) {
          method->params.push_back(consume(TokenType::Identifier, "Expected parameter name.").text);
        }
      }
      consume(TokenType::RParen, "Expected ')' after method parameters.");
      method->body = parseBlock();
      type->methods.push_back(std::move(method));
      continue;
    }

    failHere("Expected class member (val, ctor, dtor, func).");
  }

  consume(TokenType::RBracket, "Expected ']' to close class declaration.");
  return type;
}

std::unique_ptr<ExprNode> Parser::parseExpression() {
  return parseComparison();
}

std::unique_ptr<ExprNode> Parser::parseComparison() {
  auto left = parseAddition();

  while (check(TokenType::EqualEqual) || check(TokenType::BangEqual) ||
    check(TokenType::Less) || check(TokenType::LessEqual) ||
    check(TokenType::Greater) || check(TokenType::GreaterEqual)) {
    const Token op = tokens[pos++];
    auto expr = std::make_unique<BinaryExprNode>();
    expr->op = op.text;
    expr->left = std::move(left);
    expr->right = parseAddition();
    left = std::move(expr);
  }

  return left;
}

std::unique_ptr<ExprNode> Parser::parseAddition() {
  auto left = parseMultiplication();

  while (check(TokenType::Plus) || check(TokenType::Minus)) {
    const Token op = tokens[pos++];
    auto expr = std::make_unique<BinaryExprNode>();
    expr->op = op.text;
    expr->left = std::move(left);
    expr->right = parseMultiplication();
    left = std::move(expr);
  }

  return left;
}

std::unique_ptr<ExprNode> Parser::parseMultiplication() {
  auto left = parsePrimary();

  while (check(TokenType::Star) || check(TokenType::Slash) || check(TokenType::Percent)) {
    const Token op = tokens[pos++];
    auto expr = std::make_unique<BinaryExprNode>();
    expr->op = op.text;
    expr->left = std::move(left);
    expr->right = parsePrimary();
    left = std::move(expr);
  }

  return left;
}

std::unique_ptr<ExprNode> Parser::parsePrimary() {
  auto left = parseAtom();

  while (true) {
    if (match(TokenType::LBracket)) {
      auto idx = parseExpression();
      consume(TokenType::RBracket, "Expected ']' after index expression.");
      auto indexExpr = std::make_unique<IndexExprNode>();
      indexExpr->collection = std::move(left);
      indexExpr->index = std::move(idx);
      left = std::move(indexExpr);
      continue;
    }
    if (match(TokenType::Dot)) {
      const Token& prop = consume(TokenType::Identifier, "Expected property name after '.'.");
      if (match(TokenType::LParen)) {
        auto call = std::make_unique<MethodCallNode>();
        call->object = std::move(left);
        call->method = prop.text;
        if (!check(TokenType::RParen)) {
          call->args.push_back(parseExpression());
          while (match(TokenType::Comma)) {
            call->args.push_back(parseExpression());
          }
        }
        consume(TokenType::RParen, "Expected ')' after method arguments.");
        left = std::move(call);
      } else {
        auto access = std::make_unique<PropertyAccessNode>();
        access->object = std::move(left);
        access->property = prop.text;
        left = std::move(access);
      }
      continue;
    }
    break;
  }

  return left;
}

std::unique_ptr<ExprNode> Parser::parseAtom() {
  if (match(TokenType::Minus)) {
    auto operand = parseAtom();
    auto unary = std::make_unique<UnaryMinusNode>();
    unary->operand = std::move(operand);
    return unary;
  }

  if (check(TokenType::Number)) {
    const Token token = consume(TokenType::Number, "Expected number.");
    auto expr = std::make_unique<LiteralNode>();
    expr->kind = LiteralNode::Kind::Int;
    expr->value = token.text;
    return expr;
  }

  if (check(TokenType::Float)) {
    const Token token = consume(TokenType::Float, "Expected float.");
    auto expr = std::make_unique<LiteralNode>();
    expr->kind = LiteralNode::Kind::Float;
    expr->value = token.text;
    return expr;
  }

  if (check(TokenType::String)) {
    const Token token = consume(TokenType::String, "Expected string.");
    auto expr = std::make_unique<LiteralNode>();
    expr->kind = LiteralNode::Kind::String;
    expr->value = token.text;
    return expr;
  }

  if (check(TokenType::True) || check(TokenType::False)) {
    const bool isTrue = check(TokenType::True);
    consume(isTrue ? TokenType::True : TokenType::False, "Expected boolean literal.");
    auto expr = std::make_unique<LiteralNode>();
    expr->kind = LiteralNode::Kind::Bool;
    expr->value = isTrue ? "true" : "false";
    return expr;
  }

  if (check(TokenType::On) || check(TokenType::Off)) {
    const bool isOn = check(TokenType::On);
    consume(isOn ? TokenType::On : TokenType::Off, "Expected boolean literal.");
    auto expr = std::make_unique<LiteralNode>();
    expr->kind = LiteralNode::Kind::Bool;
    expr->value = isOn ? "true" : "false";
    return expr;
  }

  if (check(TokenType::Identifier)) {
    const Token token = consume(TokenType::Identifier, "Expected identifier.");
    if (match(TokenType::LParen)) {
      auto call = std::make_unique<CallNode>();
      call->callee = token.text;
      if (!check(TokenType::RParen)) {
        call->args.push_back(parseExpression());
        while (match(TokenType::Comma)) {
          call->args.push_back(parseExpression());
        }
      }
      consume(TokenType::RParen, "Expected ')' after function call.");
      return call;
    }
    auto var = std::make_unique<VariableNode>();
    var->name = token.text;
    return var;
  }

  if (match(TokenType::LParen)) {
    auto expr = parseExpression();
    consume(TokenType::RParen, "Expected ')' after expression.");
    return expr;
  }

  if (match(TokenType::LBracket)) {
    auto arr = std::make_unique<ArrayLiteralNode>();
    std::string expectedType;
    if (!check(TokenType::RBracket)) {
      auto first = parseExpression();
      expectedType = inferExprKind(first.get());
      arr->elements.push_back(std::move(first));
      while (match(TokenType::Comma)) {
        auto next = parseExpression();
        const std::string nextType = inferExprKind(next.get());
        if (!expectedType.empty() && !nextType.empty() && expectedType != nextType) {
          failHere("Mixed types in array are not allowed.");
        }
        arr->elements.push_back(std::move(next));
      }
    }
    consume(TokenType::RBracket, "Expected ']' after array literal.");
    return arr;
  }

  failHere("Expected expression.");
}

std::string Parser::inferExprKind(const ExprNode* expr) const {
  if (const auto* lit = dynamic_cast<const LiteralNode*>(expr)) {
    if (lit->kind == LiteralNode::Kind::Int) return "int";
    if (lit->kind == LiteralNode::Kind::Float) return "float";
    if (lit->kind == LiteralNode::Kind::String) return "string";
    if (lit->kind == LiteralNode::Kind::Bool) return "bool";
  }
  if (dynamic_cast<const ArrayLiteralNode*>(expr)) return "array";
  return "";
}

std::unique_ptr<ProgramNode> Parser::parse(){
  auto program = std::make_unique<ProgramNode>();
  while (!check(TokenType::VoidMain) && !isAtEnd()) {
    if (check(TokenType::Identifier) && current().text == "add") {
      consume(TokenType::Identifier, "Expected 'add'.");
      consume(TokenType::Identifier, "Expected 'library'.");
      const Token& libToken = consume(TokenType::Identifier, "Expected library name.");
      program->imports.push_back(libToken.text);
      continue;
    }
    if (check(TokenType::Val)) {
      auto stmt = parseVarDecl();
      program->topLevelVars.push_back(std::move(stmt));
      continue;
    }
    if (check(TokenType::Set)) {
      auto stmt = parseAssign();
      program->topLevelVars.push_back(std::move(stmt));
      continue;
    }
    if (check(TokenType::Func)) {
      program->functions.push_back(parseFunction());
      continue;
    }
    if (check(TokenType::Struct)) {
      program->structs.push_back(parseStruct());
      continue;
    }
    if (check(TokenType::Class)) {
      program->classes.push_back(parseClass());
      continue;
    }
    if (check(TokenType::Import)) {
      consume(TokenType::Import, "Expected 'import'.");
      const Token& libToken = consume(TokenType::Identifier, "Expected module name.");
      program->imports.push_back(libToken.text);
      continue;
    }
    failHere("Expected top-level declaration (func, struct, class, add library, use framework, or void.main).");
  }

  consume(TokenType::VoidMain, "Expected 'void.main' entry point.");
  program->mainBlock = parseBlock();
  consume(TokenType::End, "Unexpected tokens after program end.");
  return program;
}