#pragma once
#include <cstddef>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "af/atom.hh"
#include "af/integer.hh"

namespace af {

struct Function;
struct Interpreter;

using PrimitiveTypeID = std::size_t;

struct PrimitiveTypeInfo final {
 private:
  /// @brief maps names to primitive type infos
  static std::map<std::string, PrimitiveTypeInfo *> *getRegistry() {
    static auto map = new std::map<std::string, PrimitiveTypeInfo *>();
    return map;
  }

  /// @brief maps PrimitiveTypeIDs to their infos
  static std::vector<PrimitiveTypeInfo *> *all() {
    static auto vector = new std::vector<PrimitiveTypeInfo *>();
    return vector;
  }

  PrimitiveTypeInfo() = delete;

  PrimitiveTypeInfo(PrimitiveTypeID i, std::string &&n)
      : id(i), name(n) {}

 public:
  const PrimitiveTypeID id;
  const std::string name;
  std::map<Atom *, std::vector<Function>> methods;

  /// @brief Retrieves or creates a PrimitiveType with the given name
  /// @param name
  /// @return
  static PrimitiveTypeInfo *get(const std::string &name) {
    auto byName = getRegistry();
    auto byID = all();
    auto iter = byName->find(name);
    if (iter != byName->end()) {
      return iter->second;
    }
    auto info = new PrimitiveTypeInfo(byID->size(), std::string(name));
    byID->push_back(info);
    byName->insert(std::make_pair(info->name, info));
    return info;
  }

  std::vector<Function> getMethods(Atom *name) const;
};

struct AnyType final {
  bool operator==(const AnyType &other) const { return true; }
};

inline std::ostream &operator<<(std::ostream &out, AnyType x) {
  return out << "<AnyType>";
}

struct PrimitiveType final {
  PrimitiveTypeInfo *info;
  PrimitiveType() = delete;
  PrimitiveType(std::string &&name) : info(PrimitiveTypeInfo::get(std::move(name))) {}
  bool operator==(const PrimitiveType &other) const { return info == other.info; }
  void add(Function function);
};

inline std::ostream &operator<<(std::ostream &out, PrimitiveType type) {
  return out << "<Type " << type.info->name << ">";
}

struct FunctionType;

/// @brief Type of an AF Value. Movable and copyable, but copies may be expensive.
using Type = std::variant<AnyType, PrimitiveType, FunctionType>;

struct FunctionType final {
  std::vector<Type> inputs;
  std::vector<Type> outputs;

  FunctionType(std::vector<Type> &&ins, std::vector<Type> &&outs)
      : inputs(std::move(ins)), outputs(std::move(outs)) {}

  bool operator==(const FunctionType &other) const {
    return inputs == other.inputs && outputs == other.outputs;
  }
};

inline std::ostream &operator<<(std::ostream &out, Type type);

inline std::ostream &operator<<(std::ostream &out, FunctionType type) {
  out << "<FunctionType";
  for (const auto &input : type.inputs) {
    out << " " << input;
  }
  out << " =>";
  for (const auto &output : type.outputs) {
    out << " " << output;
  }
  out << ">";
  return out;
}

inline std::ostream &operator<<(std::ostream &out, Type type) {
  return std::visit([&](auto &v) -> std::ostream & { return out << v; }, type);
}

static_assert(std::is_move_constructible_v<Type>);
static_assert(std::is_copy_constructible_v<Type>);
static_assert(std::is_copy_assignable_v<Type>);
static_assert(std::is_move_assignable_v<Type>);

/// @brief AF Value
/// Value are movable and copyable (copies may be expensive).
using Value = std::variant<
    Type,
    bool,
    integer,
    double,
    Atom *,

    // TODO: Consider using shared_ptr<string> instead of just string.
    std::string,

    Function>;

struct FunctionData final {
  Atom *name;
  FunctionType type;
  std::function<void(Interpreter &)> body;
  FunctionData(Atom *n, FunctionType &&t, std::function<void(Interpreter &)> &&b)
      : name(n), type(std::move(t)), body(std::move(b)) {}
};

struct Function final {
  std::shared_ptr<FunctionData> data;
  Function(Atom *n, FunctionType &&t, std::function<void(Interpreter &)> &&b)
      : data(std::make_shared<FunctionData>(n, std::move(t), std::move(b))) {}
  Function(const std::string &n, FunctionType &&t, std::function<void(Interpreter &)> &&b)
      : Function(intern(n), std::move(t), std::move(b)) {}
  bool operator==(const Function &other) const { return data == other.data; }
};

inline std::ostream &operator<<(std::ostream &out, const Function &function) {
  return out << "<Function " << function.data->name->string << ">";
}

static_assert(std::is_move_constructible_v<Value>);
static_assert(std::is_copy_constructible_v<Value>);
static_assert(std::is_copy_assignable_v<Value>);
static_assert(std::is_move_assignable_v<Value>);

inline std::ostream &operator<<(std::ostream &out, Value value) {
  return std::visit(
      [&](auto &v) -> std::ostream & {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, bool>) {
          return out << (v ? "true" : "false");
        } else {
          return out << v;
        }
      },
      value);
}

inline Type getTypeType() {
  static auto type = PrimitiveType("Type");
  return type;
}

inline PrimitiveType getBoolType() {
  static auto type = PrimitiveType("Bool");
  return type;
}

inline PrimitiveType getIntType() {
  static auto type = PrimitiveType("Int");
  return type;
}

inline PrimitiveType getDoubleType() {
  static auto type = PrimitiveType("Double");
  return type;
}

inline PrimitiveType getAtomType() {
  static auto type = PrimitiveType("Atom");
  return type;
}

inline PrimitiveType getStringType() {
  static auto type = PrimitiveType("String");
  return type;
}

inline Type typeOf(const Type &x) {
  return getTypeType();
}

inline Type typeOf(bool x) {
  return getBoolType();
}

inline Type typeOf(integer x) {
  return getIntType();
}

inline Type typeOf(double x) {
  return getDoubleType();
}

inline Type typeOf(Atom *x) {
  return getAtomType();
}

inline Type typeOf(const std::string &x) {
  return getStringType();
}

inline Type typeOf(const Value &value) {
  return std::visit([&](const auto &v) -> Type { return typeOf(v); }, value);
}

inline std::vector<Function> getMethodsForType(Type type, Atom *methodName) {
  return std::visit(
      [&](const auto &t) -> std::vector<Function> {
        using T = std::decay_t<decltype(t)>;
        if constexpr (std::is_same_v<T, PrimitiveType>) {
          return t.info->getMethods(methodName);
        } else {
          return {};
        }
      },
      type);
}

inline std::vector<Function> PrimitiveTypeInfo::getMethods(Atom *methodName) const {
  auto iter = methods.find(methodName);
  if (iter == methods.end()) {
    return {};
  }
  return iter->second;
}

inline void PrimitiveType::add(Function function) {
  auto n = function.data->name;
  auto iter = info->methods.find(n);
  if (iter != info->methods.end()) {
    iter->second.push_back(function);
  } else {
    info->methods.insert(std::make_pair(n, std::vector({function})));
  }
}

inline bool isinstance(const Value &value, const Type &type) {
  return std::visit(
      [&](const auto &t) -> bool {
        using T = std::decay_t<decltype(t)>;
        if constexpr (std::is_same_v<T, AnyType>) {
          return true;
        } else if constexpr (std::is_same_v<T, PrimitiveType> ||
                             std::is_same_v<T, FunctionType>) {
          return typeOf(value) == type;
        }
      },
      type);
}

inline bool functionMatchesStack(const FunctionType &type, const std::vector<Value> &stack) {
  if (type.inputs.size() > stack.size()) {
    return false;
  }
  for (size_t i = 0; i < type.inputs.size(); i++) {
    if (!isinstance(stack[stack.size() - type.inputs.size() + i], type.inputs[i])) {
      return false;
    }
  }
  return true;
}

}  // namespace af
