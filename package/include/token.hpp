#pragma once
#include <string>

enum class TokenType {
  VoidMain,
  Val,
  Set,
  Obj,
  If,
  Else,
  While,
  For,
  Func,
  Return,
  Struct,
  Class,
  Ctor,
  Dtor,
  CompOut,
  CompIn,
  InKw,
  Identifier,
  Number,
  String,
  True,
  False,
  On,
  Off,
  Float,
  Plus,
  PlusPlus,
  Minus,
  MinusMinus,
  Star,
  Slash,
  Equal,
  EqualEqual,
  BangEqual,
  Less,
  LessEqual,
  Greater,
  GreaterEqual,
  Comma,
  Colon,
  Dot,
  LBrace,
  RBrace,
  LParen,
  RParen,
  LBracket,
  RBracket,
  End
};

struct Token {
  TokenType type;
  std::string text;
  int line = 1;
};