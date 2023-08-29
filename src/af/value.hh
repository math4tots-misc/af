#pragma once
#include <cstddef>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "af/atom.hh"
#include "af/integer.hh"

namespace af {

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
};

inline std::ostream &operator<<(std::ostream &out, PrimitiveType type) {
  return out << "<Type " << type.info->name << ">";
}

using Type = std::variant<AnyType, PrimitiveType>;

inline std::ostream &operator<<(std::ostream &out, Type type) {
  return std::visit([&](auto &v) -> std::ostream & { return out << v; }, type);
}

/// @brief AF Value
/// Value are movable and copyable (copies may be expensive).
using Value = std::variant<
    Type,
    bool,
    integer,
    double,
    Atom *,

    // TODO: Consider using shared_ptr<string> instead of just string.
    std::string>;

static_assert(std::is_move_constructible_v<Value>);
static_assert(std::is_copy_constructible_v<Value>);
static_assert(std::is_copy_assignable_v<Value>);
static_assert(std::is_move_assignable_v<Value>);

inline std::ostream &operator<<(std::ostream &out, Atom *atom) {
  return out << ":" << atom->string;
}

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

inline Type getBoolType() {
  static auto type = PrimitiveType("Bool");
  return type;
}

inline Type getIntType() {
  static auto type = PrimitiveType("Int");
  return type;
}

inline Type getDoubleType() {
  static auto type = PrimitiveType("Double");
  return type;
}

inline Type getAtomType() {
  static auto type = PrimitiveType("Atom");
  return type;
}

inline Type getStringType() {
  static auto type = PrimitiveType("String");
  return type;
}

inline Type typeOfStaticValue(Type x) {
  return getTypeType();
}

inline Type typeOfStaticValue(bool x) {
  return getBoolType();
}

inline Type typeOfStaticValue(integer x) {
  return getIntType();
}

inline Type typeOfStaticValue(double x) {
  return getDoubleType();
}

inline Type typeOfStaticValue(Atom *x) {
  return getAtomType();
}

inline Type typeOfStaticValue(const std::string &x) {
  return getStringType();
}

inline Type typeOf(const Value &value) {
  return std::visit([&](const auto &v) -> Type { return typeOfStaticValue(v); }, value);
}

}  // namespace af
