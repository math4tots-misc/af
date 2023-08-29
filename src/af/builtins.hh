#pragma once

#include <iostream>
#include <sstream>

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
  ip.globals.insert(std::make_pair<Atom *, Type>(intern("Int"), getIntType()));
  ip.globals.insert(std::make_pair<Atom *, Type>(intern("Double"), getDoubleType()));
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
      }));

  ip.globals.insert(mkfunc(
      "print-stack",
      FunctionType({}, {}),
      [](Interpreter &ix) {
        ix.printStack(std::cout);
      }));

  ip.globals.insert(mkfunc(
      "dup",
      FunctionType({AnyType{}}, {AnyType{}, AnyType{}}),
      [](Interpreter &ix) {
        ix.push(Value(ix.stack.back()));
      }));

  // Defines functions
  ip.globals.insert(mkfunc(
      ":",
      FunctionType({getAtomType()}, {}),
      [](Interpreter &ix) {
        auto name = ix.pop<Atom *>();
        ix.pushStack();
        ix.mode = CompileMode([=](Interpreter &ii) {
          auto defn = ii.popStack();
          size_t i = 0;
          auto inputs = std::vector<Type>();
          while (i < defn.size() && defn[i] != Value(getAtomArrow())) {
            auto typeName = std::get<Atom *>(defn[i]);
            auto typePtr = ii.lookup<Type *>(typeName);
            if (!typePtr) {
              std::stringstream ss;
              ss << "Type " << typeName->string << " not found (for input type)";
              throw Error(ss.str());
            }
            inputs.push_back(*typePtr);
            i++;
          }
          auto inputsCount = inputs.size();
          if (i < defn.size() && defn[i] == Value(getAtomArrow())) {
            i++;
          }
          auto outputs = std::vector<Type>();
          while (i < defn.size() && std::holds_alternative<Atom *>(defn[i]) &&
                 defn[i] != Value(getAtomColon()) &&
                 defn[i] != Value(getAtomSemicolon())) {
            auto typeName = std::get<Atom *>(defn[i]);
            auto typePtr = ii.lookup<Type *>(typeName);
            if (!typePtr) {
              std::stringstream ss;
              ss << "Type " << typeName->string << " not found (for output type)";
              throw Error(ss.str());
            }
            outputs.push_back(*typePtr);
            i++;
          }
          auto type = FunctionType(std::move(inputs), std::move(outputs));

          // TODO: Figure out what the intent of the type annotation syntax was
          // Seems a bit funky to me right now, and there aren't that many examples.

          // Explicit map of input -> output values
          std::map<std::vector<Value>, std::vector<Value>> valueMap;

          while (i < defn.size() && defn[i] == Value(getAtomColon())) {
            i++;
            auto inputValues = std::vector<Value>();
            while (i < defn.size() && defn[i] != Value(getAtomArrow())) {
              inputValues.push_back(defn[i++]);
            }
            if (i < defn.size() && defn[i] == Value(getAtomArrow())) {
              i++;  // skip the arrow
            }
            auto outputValues = std::vector<Value>();
            while (i < defn.size() &&
                   defn[i] != Value(getAtomColon()) &&
                   defn[i] != Value(getAtomSemicolon())) {
              outputValues.push_back(defn[i++]);
            }
          }

          // Body of the function. For now only one is supported.
          // TODO: we might want to have multiple bodies based on
          // pattern matching.
          // TODO: Store the body as Tokens (will require some
          // factoring to accept the Tokens before they are converted)
          std::vector<Value> body;
          if (i < defn.size() && defn[i] == Value(getAtomSemicolon())) {
            i++;
          }
          while (i < defn.size() && defn[i] != Value(getAtomDot())) {
            body.push_back(std::move(defn[i++]));
          }

          auto fn = Function(
              name, std::move(type),
              [inputsCount,
               valueMap = std::move(valueMap),
               body = std::move(body)](Interpreter &i2) {
                if (valueMap.size()) {
                  // TODO: avoid popping and pushing back.
                  auto args = i2.popn(inputsCount);
                  auto iter = valueMap.find(args);
                  if (iter != valueMap.end()) {
                    i2.pushn(std::vector(iter->second));
                    return;
                  } else {
                    i2.pushn(std::move(args));
                  }
                }
                for (auto &value : body) {
                  i2.feed(value);
                }
              });
          ii.globals.insert(std::make_pair(name, std::move(fn)));
          ii.mode = InterpretMode();
        });
      }));
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
  intType.add(Function(
      "-",
      FunctionType({getIntType(), getIntType()}, {getIntType()}),
      [](Interpreter &ip) {
        auto b = ip.pop<integer>();
        auto a = ip.pop<integer>();
        ip.push(a - b);
      }));
  intType.add(Function(
      "*",
      FunctionType({getIntType(), getIntType()}, {getIntType()}),
      [](Interpreter &ip) {
        auto b = ip.pop<integer>();
        auto a = ip.pop<integer>();
        ip.push(a * b);
      }));
  intType.add(Function(
      "%",
      FunctionType({getIntType(), getIntType()}, {getIntType()}),
      [](Interpreter &ip) {
        auto b = ip.pop<integer>();
        auto a = ip.pop<integer>();
        ip.push(a % b);
      }));
}

}  // namespace af
