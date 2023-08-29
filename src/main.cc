#include <iostream>

#include "af/lexer.hh"

using namespace af;

int main() {
  Lexer lexer("Hello world. 5. 5.5");
  for (;;) {
    auto optionalToken = lexer.next();
    if (!optionalToken.has_value()) {
      break;
    }
    auto token = optionalToken.value();
    std::cout << token << std::endl;
  }
}
