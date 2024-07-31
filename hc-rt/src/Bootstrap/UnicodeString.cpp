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
