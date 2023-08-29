#pragma once
#include <map>
#include <optional>
#include <sstream>
#include <type_traits>
#include <vector>

#include "af/atom.hh"
#include "af/err.hh"
#include "af/lexer.hh"
#include "af/value.hh"

namespace af {

inline std::optional<Function> getMethodForType(const Type &type, Atom *name, Interpreter &ip);

struct Interpreter final {
  enum class Mode {
    INTERPRET,
    COMPILE,
  };
  std::map<Atom *, Value> globals;
  std::vector<Value> stack;
  Mode mode = Mode::INTERPRET;

  template <class T>
  T pop();

  template <>
  Value pop<Value>() {
    auto value = std::move(stack.back());
    stack.pop_back();
    return value;  // No need to move: RVO/copy elision
  }

  template <>
  integer pop<integer>() {
    return std::visit(
        [&](const auto &&value) -> integer {
          using T = std::decay_t<decltype(value)>;
          if constexpr (std::is_same_v<T, integer>) {
            return value;
          } else {
            std::stringstream ss;
            ss << "Expected integer but got " << typeOf(value);
            throw Error(ss.str());
          }
        },
        pop<Value>());
  }

  void push(Value value) {
    stack.push_back(value);
  }

  void apply(const Function &function) {
    if (!functionMatchesStack(function.data->type, stack)) {
      std::stringstream ss;
      ss << "Function " << function.data->name << " does not match signature on stack";
      throw Error(ss.str());
    }
    function.data->body(*this);
  }

  std::ostream &printStack(std::ostream &out) const {
    out << "=== PRINTING VALUE STACK === " << std::endl;
    out << "    CONTAINING " << stack.size() << " ELEMENTS" << std::endl;
    for (auto i = stack.size(); i; i--) {
      out << stack[i - 1] << std::endl;
      out << "  " << typeOf(stack[i - 1]) << std::endl;
    }
    out << "===    END OF STACK      ===" << std::endl;
    return out;
  }

  bool feed(Lexer &lexer) {
    auto opt = lexer.next();
    if (opt.has_value()) {
      feed(opt.value());
      return true;
    }
    return false;
  }

  /// @brief Feed a token to the interpreter
  /// @param token
  void feed(const Token &token) {
    static const Atom *ATOM_DOT = intern(".");
    switch (mode) {
      case Mode::INTERPRET:
        std::visit(
            [&](const auto &t) {
              using T = std::decay_t<decltype(t)>;
              if constexpr (std::is_same_v<T, IntegerToken> ||
                            std::is_same_v<T, DoubleToken> ||
                            std::is_same_v<T, StringToken>) {
                stack.push_back(t.value);
              } else if constexpr (std::is_same_v<T, AtomToken>) {
                auto atom = t.value;

                // Check for methods on TOS value
                if (stack.size() > 0) {
                  auto opt = getMethodForType(typeOf(stack.back()), atom, *this);
                  if (opt.has_value()) {
                    apply(opt.value());
                    return;
                  }
                }

                // Check if this is a globally identified value
                auto globalIter = globals.find(atom);
                if (globalIter != globals.end()) {
                  std::visit(
                      [&](const auto &v) {
                        using V = std::decay_t<decltype(v)>;
                        if constexpr (std::is_same_v<V, Function>) {
                          apply(v);
                        } else {
                          // If not a function, just add it to the stack
                          stack.push_back(v);
                        }
                      },
                      globalIter->second);
                  return;
                }

                // If the atom does not match anything, just push
                // on to top of the stack.
                stack.push_back(atom);
              }
            },
            token);
        break;
      case Mode::COMPILE:
        std::visit(
            [&](const auto &t) {
              using T = std::decay_t<decltype(t)>;
              if constexpr (std::is_same_v<T, IntegerToken> ||
                            std::is_same_v<T, DoubleToken> ||
                            std::is_same_v<T, StringToken>) {
                stack.push_back(t.value);
              } else if constexpr (std::is_same_v<T, AtomToken>) {
                auto value = t.value;
                if (value == ATOM_DOT) {
                  mode = Mode::INTERPRET;
                }
              }
            },
            token);
        break;
    }
  }
};

inline std::optional<Function> getMethodForType(const Type &type, Atom *name, Interpreter &ip) {
  for (auto method : getMethodsForType(type, name)) {
    if (functionMatchesStack(method.data->type, ip.stack)) {
      return {method};
    }
  }
  return std::nullopt;
}

}  // namespace af
