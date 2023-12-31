#pragma once
#include <cctype>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <variant>

#include "af/atom.hh"
#include "af/integer.hh"

namespace af {
struct AtomToken final {
  Atom *value;
  explicit AtomToken(Atom *v) : value(v) {}
  explicit AtomToken(const std::string &v) : value(intern(v)) {}
};

inline std::ostream &operator<<(std::ostream &out, const AtomToken &token) {
  return out << "AtomToken(\"" << token.value->string << "\")";
}

struct IntegerToken final {
  integer value;
  explicit IntegerToken(integer v) : value(v) {}
};

inline std::ostream &operator<<(std::ostream &out, const IntegerToken &token) {
  return out << "IntegerToken(" << token.value << ")";
}

struct DoubleToken final {
  double value;
  explicit DoubleToken(double v) : value(v) {}
};

inline std::ostream &operator<<(std::ostream &out, const DoubleToken &token) {
  return out << "DoubleToken(" << token.value << ")";
}

struct StringToken final {
  std::string value;
  explicit StringToken(std::string &&v) : value(std::move(v)) {}
};

inline std::ostream &operator<<(std::ostream &out, const StringToken &token) {
  return out << "StringToken(\"" << token.value << "\")";
}

using Token = std::variant<AtomToken, IntegerToken, DoubleToken, StringToken>;

inline std::ostream &operator<<(std::ostream &out, const Token &token) {
  std::visit([&](const auto &t) { out << t; }, token);
  return out;
}

struct Lexer final {
 private:
  std::string str;
  size_t i = 0;
  char peek = '\0';

  /// @brief Special punctuation token that does not need whitespace to be separated
  char special = 0;

 public:
  Lexer(std::string &&s) : str(std::move(s)) {
    if (str.size()) {
      peek = str[0];
    }
  }

  // Explicitly deleted because the default is broken by 'istream &in'
  Lexer &operator=(Lexer &&lx) = delete;

 private:
  static bool isSpecial(char c) {
    return c == '.' || c == ';' || c == ':';
  }

  char incr() {
    char old = peek;
    if (peek != '\0') {
      i++;
      peek = i >= str.size() ? '\0' : str[i];
    }
    return old;
  }
  bool eof() const {
    return peek == '\0';
  }

 public:
  std::optional<Token> next() {
    if (special) {
      std::string string({special});
      special = 0;
      return {AtomToken(std::move(string))};
    }

    while (!eof() && (std::isspace(peek) || peek == '#')) {
      if (peek == '#') {
        while (!eof() && peek != '\n') {
          incr();
        }
      } else {
        incr();
      }
    }

    if (eof()) {
      return std::nullopt;
    }

    if (peek == '"') {
      incr();
      std::string out;
      while (!eof() && peek != '"') {
        if (peek == '\\') {
          incr();
          if (!eof()) {
            switch (incr()) {
              case 't':
                out.push_back('\t');
                break;
              case 'r':
                out.push_back('\r');
                break;
              case 'n':
                out.push_back('\n');
                break;
            }
          }
        } else {
          out.push_back(incr());
        }
      }
      if (!eof() && peek == '"') {
        incr();
      }
      return {StringToken(std::move(out))};
    }

    std::string value;
    if (std::isdigit(peek)) {
      bool dot = false;
      value.push_back(incr());
      while (!eof() && std::isdigit(peek)) {
        value.push_back(incr());
      }
      if (!eof() && peek == '.') {
        dot = true;
        value.push_back(incr());
        while (!eof() && std::isdigit(peek)) {
          value.push_back(incr());
        }
      }

      // We only presume to have a number token if the entire string
      // matches the number pattern. If there are any extraneous non-whitespace
      // characters, we process the token as an atom
      if (eof() || std::isspace(peek) || isSpecial(peek)) {
        // If the number ends with a '.', treat the '.' as a separate token.
        if (value.back() == '.') {
          dot = false;
          value.pop_back();
          special = '.';
        }
        if (dot) {
          return {DoubleToken(std::stod(value))};
        }
        return {IntegerToken(std::stoll(value))};
      }

      // NOTE: There's no need to rewind (i.e. i = j)
    }

    while (!eof() && !std::isspace(peek)) {
      value.push_back(incr());
    }

    // If the atom ends with a special character, address this.
    if (value.size() > 1 && isSpecial(value.back())) {
      special = value.back();
      value.pop_back();
    }
    return {AtomToken(std::move(value))};
  }
};
}  // namespace af
