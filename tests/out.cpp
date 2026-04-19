#include <iostream>
#include <string>

#include <vector>

#include <stdexcept>

auto add(auto a, auto b) {
  return (a + b);
}

int main() {
  std::cout.setf(std::ios::boolalpha);
  int x = 5;
  auto y = add(x, 4);
  std::cout << y << std::endl;
  return 0;
}
