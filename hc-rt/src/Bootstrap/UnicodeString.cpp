//===- Bootstrap/UnicodeString.cpp ----------------------------------===//
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

#include <Bootstrap/UnicodeString.hpp>
#include <Common/Strings.hpp>
#include <Common/PtrRange.hpp>

using namespace hc;
using namespace hc::bootstrap;

__intrnl wchar_t __gnull = L'\0';

UnicodeString UnicodeString::New(wchar_t* S) {
  __hc_invariant(S != nullptr);
  UnicodeString new_ustr;
  usize str_len = __wstrlen(S);
  new_ustr.buffer = S;
  new_ustr.__size = str_len * 2;
  new_ustr.__size_max = (str_len + 1) * 2;
  return new_ustr;
}

UnicodeString UnicodeString::New(wchar_t* S, usize max) {
  __hc_invariant(S != nullptr);
  UnicodeString new_ustr;
  new_ustr.buffer = S;
  new_ustr.__size = __wstrlen(S) * 2;
  new_ustr.__size_max = max * 2;
  __hc_invariant(new_ustr.__size <= new_ustr.__size_max);
  return new_ustr;
}

[[gnu::flatten]]
UnicodeString UnicodeString::New(PtrRange<wchar_t> R) {
  __hc_invariant(!R.isEmpty());
  return UnicodeString::New(R.data(), R.size());
}

bool UnicodeString::isEqual(const UnicodeString& rhs) const {
  if __expect_false(!this->buffer || !rhs.buffer)
    return false;
  if (this->__size != rhs.__size) 
    return false;
  int ret = __wstrncmp(this->buffer, rhs.buffer, this->getSize());
  return (ret == 0);
}

// "Mutators"

com::PtrRange<wchar_t> UnicodeString::intoRange() const {
  const int sz = getSize();
  if __expect_false(sz == 0)
    return {};
  __hc_invariant(buffer != nullptr);
  return {buffer, buffer + sz};
}

com::PtrRange<const wchar_t> UnicodeString::intoImmRange() const {
  return this->intoRange().intoImmRange();
}

const wchar_t& UnicodeString::frontSafe() const {
  if __expect_false(getSize() == 0)
    return __gnull;
  __hc_invariant(buffer != nullptr);
  return buffer[0];
}

const wchar_t& UnicodeString::backSafe() const {
  if __expect_false(getSize() == 0)
    return __gnull;
  __hc_invariant(buffer != nullptr);
  return buffer[getSize() - 1];
}

#include <Common/Casting.hpp>
#include <Common/Function.hpp>
#include <Common/MMatch.hpp>

i32 hc::bootstrap::UnicodeStringToInteger(
 UnicodeString S, u32& out, u32 base) {
  // TODO: Swap out when Format/* is complete.
  const wchar_t* I = S.buffer;
  const wchar_t* E = I + S.getSize();
  bool is_negative = false;

  if __expect_false(MMatch(base).isnt(0, 2, 8, 10, 16)) {
    // STATUS_INVALID_PARAMETER
    return 0xC000000D;
  }

  auto in_range = [&I, &E]() -> bool {
    return __likely_true(I != E);
  };
  auto advance = [&]() -> bool {
    if __expect_true(in_range())
      ++I;
    return in_range();
  };

  auto curr = [&]() -> wchar_t {
    return in_range() ? *I : L'\0';
  };
  auto M = [&]() -> MMatch<wchar_t> {
    return MMatch(curr());
  };
  auto next = [&]() -> wchar_t {
    return advance() ? *I : L'\0';
  };

  while (M().is(L' ', L'\t', L'\n', L'\r')) {
    (void) advance();
  }

  if __likely_false(!in_range()) {
    // STATUS_INVALID_PARAMETER
    return 0xC000000D;
  }

  out = 0;
  if (M().is(L'-', L'+')) {
    if (curr() == L'-')
      is_negative = true;
    (void) advance();
  }

  if (base == 0) $scope {
    base = 10;
    if (curr() != L'0')
      break;
    switch (next()) {
     case L'b':
      base = 2;
      break;
     case L'o':
      base = 8;
      break;
     case L'x':
      base = 16;
      break;
     default:
    }
    if (base != 10)
      advance();
  };

  i32 tmp = 0;
  auto conv = [&, base](com::Function<i32(wchar_t)> F) {
    while (in_range()) {
      const i32 I = F(curr());
      if (I == -1)
        return;
      tmp *= base;
      tmp += I;
      advance();
    }
  };

  if (base == 2) {
    conv([](wchar_t WC) -> i32 {
      if (MMatch(WC).is(L'0', L'1'))
        return WC - L'0';
      return -1;
    });
  } else if (base == 8) {
    conv([](wchar_t WC) -> i32 {
      if (MMatch(WC).iin(L'0', L'7'))
        return WC - L'0';
      return -1;
    });
  } else if (base == 10) {
    conv([](wchar_t WC) -> i32 {
      if (MMatch(WC).iin(L'0', L'9'))
        return WC - L'0';
      return -1;
    });
  } else {
    conv([](wchar_t WC) -> i32 {
      if (MMatch(WC).iin(L'0', L'9'))
        return WC - L'0';
      else if (MMatch(WC).iin(L'A', L'F'))
        return (WC - L'A') + 10;
      else if (MMatch(WC).iin(L'a', L'f'))
        return (WC - L'a') + 10;
      return -1;
    });
  }

  out = int_cast<u32>(is_negative ? -tmp : tmp);
  return 0;
}
