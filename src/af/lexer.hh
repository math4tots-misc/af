#pragma once
#include <cctype>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <variant>

namespace af {
struct AtomToken final {
  std::string value;
  explicit AtomToken(std::string &&v) : value(std::move(v)) {}
};

inline std::ostream &operator<<(std::ostream &out, const AtomToken &token) {
  return out << "AtomToken(\"" << token.value << "\")";
}

struct NumberToken final {
  double value;
  explicit NumberToken(double v) : value(v) {}
};

inline std::ostream &operator<<(std::ostream &out, const NumberToken &token) {
  return out << "NumberToken(" << token.value << ")";
}

struct StringToken final {
  std::string value;
  explicit StringToken(std::string &&v) : value(std::move(v)) {}
};

inline std::ostream &operator<<(std::ostream &out, const StringToken &token) {
  return out << "StringToken(\"" << token.value << "\")";
}

using Token = std::variant<AtomToken, NumberToken, StringToken>;

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
  bool dotNext = false;

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
    dotNext = lx.dotNext;
  }

  // Explicitly deleted because the default is broken by 'istream &in'
  Lexer &operator=(Lexer &&lx) = delete;

 private:
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
    if (dotNext) {
      dotNext = false;
      return {AtomToken(".")};
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
      value.push_back(incr());
      while (!eof() && std::isdigit(peek)) {
        value.push_back(incr());
      }
      if (!eof() && peek == '.') {
        value.push_back(incr());
        while (!eof() && std::isdigit(peek)) {
          value.push_back(incr());
        }
      }

      // We only presume to have a number token if the entire string
      // matches the number pattern. If there are any extraneous non-whitespace
      // characters, we process the token as an atom
      if (eof() || std::isspace(peek)) {
        // If the number ends with a '.', treat the '.' as a separate token.
        if (value.back() == '.') {
          value.pop_back();
          dotNext = true;
        }
        return {NumberToken(std::stod(value))};
      }

      // NOTE: There's no need to rewind (i.e. i = j)
    }

    while (!eof() && !std::isspace(peek)) {
      value.push_back(incr());
    }

    // If the atom ends with a '.', treat the '.' as a separate token.
    if (value.back() == '.') {
      value.pop_back();
      dotNext = true;
    }
    return {AtomToken(std::move(value))};
  }
};
}  // namespace af
