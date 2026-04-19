#include "scanner.hpp"
#include <cctype>
#include <stdexcept>

Scanner::Scanner(const std::string& s):src(s){}

std::vector<Token> Scanner::scan(){
  std::vector<Token> tokens;
  auto pushToken = [&](TokenType type, const std::string& text) {
    tokens.push_back({type, text, line});
  };
  auto isIdentChar = [](char ch) {
    return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_';
  };

  while (pos < src.size()) {
    char c = src[pos];

    if (std::isspace(static_cast<unsigned char>(c))) {
      if (c == '\n') ++line;
      ++pos;
      continue;
    }

    if (c == '#') {
      while (pos < src.size() && src[pos] != '\n') {
        ++pos;
      }
      continue;
    }

    if (src.compare(pos, 9, "void.main") == 0 &&
      (pos == 0 || !isIdentChar(src[pos - 1])) &&
      (pos + 9 >= src.size() || !isIdentChar(src[pos + 9]))) {
      pushToken(TokenType::VoidMain, "void.main");
      pos += 9;
      continue;
    }

    if (src.compare(pos, 8, "comp.out") == 0 &&
      (pos == 0 || !isIdentChar(src[pos - 1])) &&
      (pos + 8 >= src.size() || !isIdentChar(src[pos + 8]))) {
      pushToken(TokenType::CompOut, "comp.out");
      pos += 8;
      continue;
    }

    if (src.compare(pos, 7, "comp.in") == 0 &&
      (pos == 0 || !isIdentChar(src[pos - 1])) &&
      (pos + 7 >= src.size() || !isIdentChar(src[pos + 7]))) {
      pushToken(TokenType::CompIn, "comp.in");
      pos += 7;
      continue;
    }

if (src.compare(pos, 2, "in") == 0 &&
      (pos == 0 || !isIdentChar(src[pos - 1])) &&
      (pos + 2 >= src.size() || !isIdentChar(src[pos + 2]))) {
      pushToken(TokenType::InKw, "in");
      pos += 2;
      continue;
    }

    if (src.compare(pos, 3, "val") == 0 &&
      (pos == 0 || !isIdentChar(src[pos - 1])) &&
      (pos + 3 >= src.size() || !isIdentChar(src[pos + 3]))) {
      pushToken(TokenType::Val, "val");
      pos += 3;
      continue;
    }

    if (src.compare(pos, 3, "set") == 0 &&
      (pos == 0 || !isIdentChar(src[pos - 1])) &&
      (pos + 3 >= src.size() || !isIdentChar(src[pos + 3]))) {
      pushToken(TokenType::Set, "set");
      pos += 3;
      continue;
    }

    if (src.compare(pos, 3, "obj") == 0 &&
      (pos == 0 || !isIdentChar(src[pos - 1])) &&
      (pos + 3 >= src.size() || !isIdentChar(src[pos + 3]))) {
      pushToken(TokenType::Obj, "obj");
      pos += 3;
      continue;
    }

    if (src.compare(pos, 2, "if") == 0 &&
      (pos == 0 || !isIdentChar(src[pos - 1])) &&
      (pos + 2 >= src.size() || !isIdentChar(src[pos + 2]))) {
      pushToken(TokenType::If, "if");
      pos += 2;
      continue;
    }

    if (src.compare(pos, 4, "else") == 0 &&
      (pos == 0 || !isIdentChar(src[pos - 1])) &&
      (pos + 4 >= src.size() || !isIdentChar(src[pos + 4]))) {
      pushToken(TokenType::Else, "else");
      pos += 4;
      continue;
    }

    if (src.compare(pos, 5, "while") == 0 &&
      (pos == 0 || !isIdentChar(src[pos - 1])) &&
      (pos + 5 >= src.size() || !isIdentChar(src[pos + 5]))) {
      pushToken(TokenType::While, "while");
      pos += 5;
      continue;
    }

    if (src.compare(pos, 3, "for") == 0 &&
      (pos == 0 || !isIdentChar(src[pos - 1])) &&
      (pos + 3 >= src.size() || !isIdentChar(src[pos + 3]))) {
      pushToken(TokenType::For, "for");
      pos += 3;
      continue;
    }

if (src.compare(pos, 7, "library") == 0 &&
      (pos == 0 || !isIdentChar(src[pos - 1])) &&
      (pos + 7 >= src.size() || !isIdentChar(src[pos + 7]))) {
      pushToken(TokenType::Identifier, "library");
      pos += 7;
      continue;
    }

    if (src.compare(pos, 4, "func") == 0 &&
      (pos == 0 || !isIdentChar(src[pos - 1])) &&
      (pos + 4 >= src.size() || !isIdentChar(src[pos + 4]))) {
      pushToken(TokenType::Func, "func");
      pos += 4;
      continue;
    }

    if (src.compare(pos, 6, "return") == 0 &&
      (pos == 0 || !isIdentChar(src[pos - 1])) &&
      (pos + 6 >= src.size() || !isIdentChar(src[pos + 6]))) {
      pushToken(TokenType::Return, "return");
      pos += 6;
      continue;
    }

    if (src.compare(pos, 4, "true") == 0 &&
      (pos == 0 || !isIdentChar(src[pos - 1])) &&
      (pos + 4 >= src.size() || !isIdentChar(src[pos + 4]))) {
      pushToken(TokenType::True, "true");
      pos += 4;
      continue;
    }

    if (src.compare(pos, 5, "false") == 0 &&
      (pos == 0 || !isIdentChar(src[pos - 1])) &&
      (pos + 5 >= src.size() || !isIdentChar(src[pos + 5]))) {
      pushToken(TokenType::False, "false");
      pos += 5;
      continue;
    }

    if (src.compare(pos, 2, "on") == 0 &&
      (pos == 0 || !isIdentChar(src[pos - 1])) &&
      (pos + 2 >= src.size() || !isIdentChar(src[pos + 2]))) {
      pushToken(TokenType::On, "on");
      pos += 2;
      continue;
    }

    if (src.compare(pos, 3, "off") == 0 &&
      (pos == 0 || !isIdentChar(src[pos - 1])) &&
      (pos + 3 >= src.size() || !isIdentChar(src[pos + 3]))) {
      pushToken(TokenType::Off, "off");
      pos += 3;
      continue;
    }

    if (src.compare(pos, 6, "struct") == 0 &&
      (pos == 0 || !isIdentChar(src[pos - 1])) &&
      (pos + 6 >= src.size() || !isIdentChar(src[pos + 6]))) {
      pushToken(TokenType::Struct, "struct");
      pos += 6;
      continue;
    }

    if (src.compare(pos, 5, "class") == 0 &&
      (pos == 0 || !isIdentChar(src[pos - 1])) &&
      (pos + 5 >= src.size() || !isIdentChar(src[pos + 5]))) {
      pushToken(TokenType::Class, "class");
      pos += 5;
      continue;
    }

    if (src.compare(pos, 4, "ctor") == 0 &&
      (pos == 0 || !isIdentChar(src[pos - 1])) &&
      (pos + 4 >= src.size() || !isIdentChar(src[pos + 4]))) {
      pushToken(TokenType::Ctor, "ctor");
      pos += 4;
      continue;
    }

    if (src.compare(pos, 4, "dtor") == 0 &&
      (pos == 0 || !isIdentChar(src[pos - 1])) &&
      (pos + 4 >= src.size() || !isIdentChar(src[pos + 4]))) {
      pushToken(TokenType::Dtor, "dtor");
      pos += 4;
      continue;
    }

    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
      const size_t start = pos;
      while (pos < src.size() && isIdentChar(src[pos])) ++pos;
      pushToken(TokenType::Identifier, src.substr(start, pos - start));
      continue;
    }

    if (std::isdigit(static_cast<unsigned char>(c))) {
      const size_t start = pos;
      while (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) ++pos;
      if (pos < src.size() && src[pos] == '.' && pos + 1 < src.size() && std::isdigit(static_cast<unsigned char>(src[pos + 1]))) {
        ++pos; // consume '.'
        while (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) ++pos;
        pushToken(TokenType::Float, src.substr(start, pos - start));
      } else {
        pushToken(TokenType::Number, src.substr(start, pos - start));
      }
      continue;
    }

    if (c == '"') {
      ++pos;
      std::string str;
      while (pos < src.size() && src[pos] != '"') {
        if (src[pos] == '\\' && pos + 1 < src.size()) {
          char escaped = src[pos + 1];
          if (escaped == 'n') { str += '\n'; }
          else if (escaped == 't') { str += '\t'; }
          else if (escaped == 'r') { str += '\r'; }
          else if (escaped == '\\') { str += '\\'; }
          else if (escaped == '"') { str += '"'; }
          else { str += src[pos + 1]; }
          pos += 2;
          continue;
        }
        str += src[pos];
        ++pos;
      }
      if (pos >= src.size()) {
        throw std::runtime_error("[Syux Error] Line " + std::to_string(line) + ": Unterminated string literal.");
      }
      pushToken(TokenType::String, str);
      ++pos;
      continue;
    }

    if (src.compare(pos, 2, "++") == 0) { pushToken(TokenType::PlusPlus, "++"); pos += 2; continue; }
    if (src.compare(pos, 2, "--") == 0) { pushToken(TokenType::MinusMinus, "--"); pos += 2; continue; }
    if (src.compare(pos, 2, "==") == 0) { pushToken(TokenType::EqualEqual, "=="); pos += 2; continue; }
    if (src.compare(pos, 2, "!=") == 0) { pushToken(TokenType::BangEqual, "!="); pos += 2; continue; }
    if (src.compare(pos, 2, "<=") == 0) { pushToken(TokenType::LessEqual, "<="); pos += 2; continue; }
    if (src.compare(pos, 2, ">=") == 0) { pushToken(TokenType::GreaterEqual, ">="); pos += 2; continue; }

    if (c == '+') { pushToken(TokenType::Plus, "+"); ++pos; continue; }
    if (c == '-') { pushToken(TokenType::Minus, "-"); ++pos; continue; }
    if (c == '*') { pushToken(TokenType::Star, "*"); ++pos; continue; }
    if (c == '/') {
      if (pos + 1 < src.size() && src[pos + 1] == '/') {
        while (pos < src.size() && src[pos] != '\n') {
          ++pos;
        }
        continue;
      }
      pushToken(TokenType::Slash, "/");
      ++pos;
      continue;
    }
    if (c == '%') { pushToken(TokenType::Percent, "%"); ++pos; continue; }
    if (c == '=') { pushToken(TokenType::Equal, "="); ++pos; continue; }
    if (c == '<') { pushToken(TokenType::Less, "<"); ++pos; continue; }
    if (c == '>') { pushToken(TokenType::Greater, ">"); ++pos; continue; }
    if (c == ',') { pushToken(TokenType::Comma, ","); ++pos; continue; }
    if (c == ':') { pushToken(TokenType::Colon, ":"); ++pos; continue; }
    if (c == '.') { pushToken(TokenType::Dot, "."); ++pos; continue; }
    if (c == '{') { pushToken(TokenType::LBrace, "{"); ++pos; continue; }
    if (c == '}') { pushToken(TokenType::RBrace, "}"); ++pos; continue; }
    if (c == '(') { pushToken(TokenType::LParen, "("); ++pos; continue; }
    if (c == ')') { pushToken(TokenType::RParen, ")"); ++pos; continue; }
    if (c == '[') { pushToken(TokenType::LBracket, "["); ++pos; continue; }
    if (c == ']') { pushToken(TokenType::RBracket, "]"); ++pos; continue; }

    throw std::runtime_error("[Syux Error] Line " + std::to_string(line) + ": Unexpected character: " + std::string(1, c));
  }

  pushToken(TokenType::End, "");
  return tokens;
}
