#pragma once

#include <sstream>
#include <iostream>

#include "af/err.hh"
#include "af/integer.hh"
#include "af/interpreter.hh"

namespace af {

inline integer toInteger(const Value &value) {
  return std::visit(
      [&](const auto &v) -> integer {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, integer>) {
          return v;
        } else if constexpr (std::is_same_v<T, double>) {
          return static_cast<integer>(v);
        } else if constexpr (std::is_same_v<T, Atom *>) {
          return std::stoll(v->string);
        } else if constexpr (std::is_same_v<T, std::string>) {
          return std::stoll(v);
        } else {
          std::stringstream ss;
          ss << "Value of type " << typeOf(v) << " could not be converted to int";
          throw Error(ss.str());
        }
      },
      value);
}

inline std::pair<Atom *, Value> mkfunc(
    const std::string &name,
    FunctionType &&type,
    std::function<void(Interpreter &)> &&body) {
  auto atom = intern(name);
  return std::make_pair(atom, Value(Function(atom, std::move(type), std::move(body))));
}

inline void initializeBuiltins(Interpreter &ip) {
  ip.globals.insert(mkfunc(
      "int",
      FunctionType({AnyType{}}, {getIntType()}),
      [](Interpreter &ix) {
        auto value = toInteger(ix.stack.back());
        ix.stack.back() = Value(value);
      }));

      ip.globals.insert(mkfunc(
        "print",
        FunctionType({AnyType{}}, {}),
        [](Interpreter &ix) {
          auto value = ix.pop<Value>();
          std::cout << value << std::endl;
        }
      ));
}

inline void initializeBuiltinMethods() {
  auto intType = getIntType();
  intType.add(Function(
      "+",
      FunctionType({getIntType(), getIntType()}, {getIntType()}),
      [](Interpreter &ip) {
        auto b = ip.pop<integer>();
        auto a = ip.pop<integer>();
        ip.push(a + b);
      }));
}

}  // namespace af
