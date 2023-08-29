#pragma once
#include <map>
#include <string>

namespace af {

/// @brief Atoms are interned strings
struct Atom final {
 private:
  /// @brief Maps strings to the corresponding Atom
  static std::map<std::string, Atom *> *getRegistry() {
    static auto map = new std::map<std::string, Atom *>();
    return map;
  }

  Atom(std::string &&v) : string(std::move(v)) {}

 public:
  const std::string string;

  static Atom *get(const std::string &s) {
    auto registry = getRegistry();
    auto iter = registry->find(s);
    if (iter != registry->end()) {
      return iter->second;
    }
    auto atom = new Atom(std::string(s));
    registry->insert(std::make_pair(s, atom));
    return atom;
  }
};

/// @brief Intern the given string and get the associated Atom
inline Atom *intern(const std::string &s) {
  return Atom::get(s);
}

inline std::ostream &operator<<(std::ostream &out, Atom *atom) {
  return out << atom->string;
}

inline Atom *getAtomDot() {
  static auto atom = intern(".");
  return atom;
}

inline Atom *getAtomArrow() {
  static auto atom = intern("->");
  return atom;
}

inline Atom *getAtomColon() {
  static auto atom = intern(":");
  return atom;
}

inline Atom *getAtomSemicolon() {
  static auto atom = intern(";");
  return atom;
}

}  // namespace af
