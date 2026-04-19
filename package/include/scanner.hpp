#pragma once
#include "token.hpp"
#include <vector>
#include <string>

class Scanner {
  std::string src;
  size_t pos=0;
  int line=1;
public:
  Scanner(const std::string&);
  std::vector<Token> scan();
};