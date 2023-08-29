#include <fstream>
#include <iostream>

#include "af/atom.hh"
#include "af/builtins.hh"
#include "af/err.hh"
#include "af/integer.hh"
#include "af/interpreter.hh"
#include "af/lexer.hh"
#include "af/value.hh"

using namespace af;

int main(int argc, char **argv) {
  Interpreter interpreter;
  initializeBuiltins(interpreter);
  initializeBuiltinMethods();
  // auto useREPL = argc > 1;
  // auto lexer = useREPL ? Lexer(std::cin) : Lexer(std::ifstream(argv[1]));

  // Lexer lexer("Hello world. 5. 5.5 123.2. \"a string literal\" foo.");
  auto ts1 = Lexer("2 int 3 int + print");
  while (interpreter.feed(ts1)) {
  }

  auto ts2 = Lexer("\"Hello world\" print");
  while (interpreter.feed(ts2)) {
  }

  auto ts3 = Lexer("a b c 1 int 2 int 5.5");
  while (interpreter.feed(ts3)) {
  }

  interpreter.printStack(std::cout);

  // for (;;) {
  //   auto opt = lexer.next();
  //   if (!opt.has_value()) {
  //     break;
  //   }
  //   auto token = opt.value();
  //   std::cout << token << std::endl;
  // }

  // auto value = Value(44);
  // auto v2 = Value(value);
  // auto v3 = Value(Type(PrimitiveType("Int")));
  // auto v4 = Value(intern("Hello"));
  // auto v5 = Value("Hello");
  // std::cout << "value = " << value << std::endl;
  // std::cout << "v2 = " << v2 << std::endl;
  // std::cout << "v3 = " << v3 << std::endl;
  // std::cout << "v4 = " << v4 << std::endl;
  // std::cout << "v5 = " << v5 << std::endl;
  // std::cout << "v5 == v4 -> " << Value(v5 == v4) << std::endl;
  // std::cout << "v4 == Value(intern(\"Hello\")) -> " << Value(v4 == Value(intern("Hello"))) << std::endl;
  // std::cout << "v5 == Value(\"Hello\") -> " << Value(v5 == Value("Hello")) << std::endl;
  // std::cout << Type(FunctionType({getIntType()}, {getStringType()})) << std::endl;
}
