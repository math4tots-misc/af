#include <iostream>

#include "af/atom.hh"
#include "af/integer.hh"
#include "af/interpreter.hh"
#include "af/lexer.hh"
#include "af/value.hh"

using namespace af;

int main() {
  Lexer lexer("Hello world. 5. 5.5 \"a string literal\"");
  for (;;) {
    auto opt = lexer.next();
    if (!opt.has_value()) {
      break;
    }
    auto token = opt.value();
    std::cout << token << std::endl;
  }

  auto value = Value(44);
  auto v2 = Value(value);
  auto v3 = Value(Type(PrimitiveType("Int")));
  auto v4 = Value(intern("Hello"));
  auto v5 = Value("Hello");
  std::cout << "value = " << value << std::endl;
  std::cout << "v2 = " << v2 << std::endl;
  std::cout << "v3 = " << v3 << std::endl;
  std::cout << "v4 = " << v4 << std::endl;
  std::cout << "v5 = " << v5 << std::endl;
  std::cout << "v5 == v4 -> " << Value(v5 == v4) << std::endl;
  std::cout << "v4 == Value(intern(\"Hello\")) -> " << Value(v4 == Value(intern("Hello"))) << std::endl;
  std::cout << "v5 == Value(\"Hello\") -> " << Value(v5 == Value("Hello")) << std::endl;
}
