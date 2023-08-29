#pragma once
#include <functional>
#include <map>
#include <optional>
#include <sstream>
#include <type_traits>
#include <variant>
#include <vector>

#include "af/atom.hh"
#include "af/err.hh"
#include "af/lexer.hh"
#include "af/value.hh"

namespace af {

struct Interpreter;

inline std::optional<Function> getMethodForType(const Type &type, Atom *name, Interpreter &ip);

/// @brief The default mode.
struct InterpretMode final {
  void run(Interpreter &ip, const Token &token);
};

/// @brief Mode for defining functions, and build other kinds of
/// data structures without immediately 'executing' each token.
struct CompileMode final {
  std::function<void(Interpreter &)> compiler;
  CompileMode(std::function<void(Interpreter &)> &&c) : compiler(std::move(c)) {}
  void run(Interpreter &ip, const Token &token);
};

using Mode = std::variant<InterpretMode, CompileMode>;

struct Interpreter final {
  std::map<Atom *, Value> globals;
  std::vector<Value> stack;
  std::vector<std::vector<Value>> stackStack;
  Mode mode = InterpretMode();
  std::vector<Mode> modeStack;

  template <class T>
  T lookup(Atom *key);

  template <>
  Value *lookup(Atom *key) {
    auto iter = globals.find(key);
    return iter != globals.end() ? &iter->second : nullptr;
  }

  template <>
  Type *lookup(Atom *key) {
    auto ptr = lookup<Value *>(key);
    if (!ptr) {
      return nullptr;
    }
    return std::visit(
        [](auto &ref) -> Type * {
          using R = std::decay_t<decltype(ref)>;
          if constexpr (std::is_same_v<R, Type>) {
            return &ref;
          } else {
            return nullptr;
          }
        },
        *ptr);
  }

  void pushStack() {
    stackStack.push_back(std::move(stack));
    stack.clear();
  }

  std::vector<Value> popStack() {
    auto ret = std::move(stack);
    stack = std::move(stackStack.back());
    stackStack.pop_back();
    return ret;
  }

  std::vector<Value> popn(size_t n) {
    if (stack.size() < n) {
      throw Error("Stackoverflow");
    }
    std::vector<Value> ret(n);
    for (size_t i = 0; i < n; i++) {
      ret[i] = std::move(stack[stack.size() - n + i]);
    }
    stack.resize(stack.size() - n);
    return ret;
  }

  void pushn(std::vector<Value> &&values) {
    for (auto &value : values) {
      stack.push_back(std::move(value));
    }
  }

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

  template <>
  Atom *pop<Atom *>() {
    return std::visit(
        [&](const auto &&value) -> Atom * {
          using T = std::decay_t<decltype(value)>;
          if constexpr (std::is_same_v<T, Atom *>) {
            return value;
          } else {
            std::stringstream ss;
            ss << "Expected Atom but got " << typeOf(value);
            throw Error(ss.str());
          }
        },
        pop<Value>());
  }

  void push(Value &&value) {
    stack.push_back(std::move(value));
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
    std::visit([&](auto &m) { m.run(*this, token); }, mode);
  }

  void feed(const Value &value) {
    // TODO: Token should really be a type of Value
    // and tokens fed during compile mode should just be stored
    // as Tokens.
    auto token = std::visit(
        [](const auto &v) -> Token {
          using V = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<V, integer>) {
            return IntegerToken(v);
          } else if constexpr (std::is_same_v<V, double>) {
            return DoubleToken(v);
          } else if constexpr (std::is_same_v<V, Atom *>) {
            return AtomToken(v);
          } else if constexpr (std::is_same_v<V, std::string>) {
            return StringToken(std::string(v)); // TODO: avoid copying
          } else {
            std::stringstream ss;
            ss << "Instances of type " << typeOf(v) << " cannot be converted to tokens";
            throw Error(ss.str());
          }
        },
        value);
    feed(token);
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

inline void InterpretMode::run(Interpreter &ip, const Token &token) {
  std::visit(
      [&](const auto &t) {
        using T = std::decay_t<decltype(t)>;
        if constexpr (std::is_same_v<T, IntegerToken> ||
                      std::is_same_v<T, DoubleToken> ||
                      std::is_same_v<T, StringToken>) {
          ip.stack.push_back(t.value);
        } else if constexpr (std::is_same_v<T, AtomToken>) {
          auto atom = t.value;

          // Check for methods on TOS value
          if (ip.stack.size() > 0) {
            auto opt = getMethodForType(typeOf(ip.stack.back()), atom, ip);
            if (opt.has_value()) {
              ip.apply(opt.value());
              return;
            }
          }

          // Check if this is a globally identified value
          auto globalIter = ip.globals.find(atom);
          if (globalIter != ip.globals.end()) {
            std::visit(
                [&](const auto &v) {
                  using V = std::decay_t<decltype(v)>;
                  if constexpr (std::is_same_v<V, Function>) {
                    ip.apply(v);
                  } else {
                    // If not a function, just add it to the stack
                    ip.stack.push_back(v);
                  }
                },
                globalIter->second);
            return;
          }

          // TODO: This just seems like a really bad idea.
          // You wanna know when you mistype something.
          //
          // If the atom does not match anything, just push
          // on to top of the stack.
          ip.stack.push_back(atom);
        }
      },
      token);
}

inline void CompileMode::run(Interpreter &ip, const Token &token) {
  std::visit(
      [&](const auto &t) {
        using T = std::decay_t<decltype(t)>;
        if constexpr (std::is_same_v<T, IntegerToken> ||
                      std::is_same_v<T, DoubleToken> ||
                      std::is_same_v<T, StringToken>) {
          ip.stack.push_back(t.value);
        } else if constexpr (std::is_same_v<T, AtomToken>) {
          auto value = t.value;
          if (value == getAtomDot()) {
            compiler(ip);
          } else {
            ip.stack.push_back(t.value);
          }
        }
      },
      token);
}

}  // namespace af
