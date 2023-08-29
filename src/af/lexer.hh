#pragma once
#include <cctype>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <variant>

#include "af/integer.hh"

namespace af {
struct AtomToken final {
  std::string value;
  explicit AtomToken(std::string &&v) : value(std::move(v)) {}
};

inline std::ostream &operator<<(std::ostream &out, const AtomToken &token) {
  return out << "AtomToken(\"" << token.value << "\")";
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

struct LexerSource final {
 private:
  std::variant<
      std::istringstream,
      std::ifstream>
      ownership;  // Hold any data required to keep istream valid
 public:
  LexerSource() = delete;
  LexerSource(std::string &&s) : ownership(std::istringstream(std::move(s))) {}
  LexerSource(std::ifstream &&fin) : ownership(std::ifstream(std::move(fin))) {}

  std::istream &get() {
    return std::visit([&](auto &v) -> std::istream & { return v; }, ownership);
  }
};

struct Lexer final {
 private:
  LexerSource src;
  std::istream &in;
  char peek = '\0';

  /// @brief Special punctuation token that does not need whitespace to be separated
  char special = 0;

 public:
  Lexer(LexerSource &&s) : src(std::move(s)), in(src.get()) {
    if (in.eof()) {
      peek = static_cast<char>(in.get());
    }
  }
  Lexer(std::string &&s) : Lexer(LexerSource(std::move(s))) {}
  Lexer(std::ifstream &&fin) : Lexer(LexerSource(std::move(fin))) {}

  // TODO: Think about whether this is correct. I'm pretty sure
  // the default move constructor is broken because of '&in'.
  Lexer(Lexer &&lx) : Lexer(std::move(lx.src)) {
    peek = lx.peek;
    special = lx.special;
  }

  // Explicitly deleted because the default is broken by 'istream &in'
  Lexer &operator=(Lexer &&lx) = delete;

 private:
  static bool isSpecial(char c) {
    return c == '.' || c == ';' || c == ':';
  }

  char incr() {
    char old = peek;
    peek = static_cast<char>(in.get());
    return old;
  }
  bool eof() const {
    return in.eof();
  }

 public:
  std::optional<Token> next() {
    if (special) {
      std::string string({special});
      special = 0;
      return {AtomToken(std::move(string))};
    }

    while (!eof() && std::isspace(peek)) {
      incr();
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
