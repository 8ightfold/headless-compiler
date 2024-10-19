//===- Phase1/ArgParser.cpp -----------------------------------------===//
//
// Copyright (C) 2024 Eightfold
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
//     limitations under the License.
//
//===----------------------------------------------------------------===//

#include <Phase1/ArgParser.hpp>
#include <Common/InlineMemset.hpp>

using namespace hc;
using namespace hc::bootstrap;

#define TAIL_CALL(...) $tail_return __VA_ARGS__(C)

namespace {

struct ArgParser;
using TailFn = void(ArgParser::*)(char C);

struct ArgParser {
  char* I;
  char* E;
  const wchar_t* Begin;
  const wchar_t* End;
  usize argCount = 0;
public:
  ArgParser(PtrRange<char> out, PtrRange<wchar_t> US) :
   I(out.begin()), E(out.end()),
   Begin(US.begin()), End(US.end()) {
  }

  __always_inline void write(char C) { *I++ = C; }

  __always_inline void advance(usize count = 1) {
    if __likely_false(count >= size()) {
      Begin = End;
      return;
    }
    Begin += count;
  }

  __always_inline usize size() const {
    return static_cast<usize>(End - Begin);
  }

  __always_inline char peek(usize at = 0) const {
    return __expect_true(at < size()) ?
      char(Begin[at]) : '\0';
  }

  __always_inline char next(usize count = 1) {
    this->advance(count);
    return this->peek();
  }

  [[clang::preserve_most, gnu::noinline]]
  bool parseEscapedSlow() {
    usize count = 0;
    while (Begin != End && peek() == '\\') {
      this->advance();
      count += 1;
    }
    
    if (peek() != '"') {
      inline_memset(I, '\\', count);
      I += count;
      return false;
    }

    bool is_quote = true;
    if ((count & 1) == 0) {
      is_quote = false;
      count -= 1;
    }

    inline_memset(I, '\\', count / 2);

    if (!is_quote) {
      this->write('"');
      I += 2;
    }

    I += count;
    return is_quote;
  }

  template <TailFn Ret>
  void parseEscaped(char C);

  void parseNormal(char C);
  void parseQuoted(char C);
  void parseWhitespace(char C);
  void parseHandle(char C);

  template <TailFn Ret = &ArgParser::parseHandle>
  void parseDispatch(char C);

  usize parse();
};

template <TailFn Ret>
void ArgParser::parseEscaped(char C) {
  const char N = peek(1);
  if (N == '\\') {
    // Handle the complex case.
    this->parseEscapedSlow();
    $tail_return (this->*Ret)(peek());
  }

  // If the escapes aren't a complex case, fallthrough here.
  if (N != '"')
    // We didn't get a [\"] sequence, so write the escape character.
    this->write('\\');
  this->write(N);
  $tail_return (this->*Ret)(next(2));
}

template <TailFn Ret>
void ArgParser::parseDispatch(char C) {
  if __likely_false(C == '\0') {
    if (*(I - 1) != '\0')
      this->write('\0');
    return;
  }
  TAIL_CALL((this->*Ret));
}

void ArgParser::parseNormal(char C) {
  if (C == '\\') {
    // We hit a [\] sequence, handle it.
    TAIL_CALL(parseEscaped<&ArgParser::parseNormal>);
  } else if (C != ' ') {
    this->write(C);
    $tail_return parseDispatch<&ArgParser::parseNormal>(next());
  }
  $tail_return parseWhitespace(next());
}

void ArgParser::parseQuoted(char C) {
  if (C == '"') {
    $tail_return parseDispatch(next());
  } else if (C != '\\') {
    this->write(C);
    $tail_return parseDispatch<&ArgParser::parseQuoted>(next());
  }
  TAIL_CALL(parseEscaped<&ArgParser::parseQuoted>);
}

void ArgParser::parseWhitespace(char C) {
  if (C != ' ') {
    // End the current sequence of characters.
    this->write('\0');
    ++this->argCount;
    TAIL_CALL(parseDispatch);
  }

  $tail_return parseDispatch<&ArgParser::parseWhitespace>(next());
}

void ArgParser::parseHandle(char C) {
  if (C == '"')
    $tail_return parseQuoted(next());
  else if (C != ' ')
    TAIL_CALL(parseNormal);
  else
    $tail_return parseWhitespace(next());
}

usize ArgParser::parse() {
  while (I != E && *I == ' ') {
    this->advance();
    ++I;
  }
  // Leading whitespace has been cleared, start parsing.
  this->parseDispatch(peek());
  __hc_invariant((E - I) >= 1);
  this->write('\0');
  return this->argCount;
}

} // namespace `anonymous`

usize xcrt::__setup_cmdline(
 PtrRange<char> cmd, UnicodeString US) {
  return ArgParser(cmd, US.intoRange()).parse();
}
