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

static constexpr wchar_t __gnull = L'\0';

Win64UnicodeString Win64UnicodeString::New(wchar_t* S) {
  __hc_invariant(S != nullptr);
  Win64UnicodeString new_ustr;
  usize str_len = __wstrlen(S);
  new_ustr.buffer = S;
  new_ustr.size = str_len * 2;
  new_ustr.size_max = (str_len + 1) * 2;
  return new_ustr;
}

Win64UnicodeString Win64UnicodeString::New(wchar_t* S, usize max) {
  __hc_invariant(S != nullptr);
  Win64UnicodeString new_ustr;
  new_ustr.buffer = S;
  new_ustr.size = __wstrlen(S) * 2;
  new_ustr.size_max = max * 2;
  __hc_invariant(new_ustr.size <= new_ustr.size_max);
  return new_ustr;
}

[[gnu::flatten]]
Win64UnicodeString Win64UnicodeString::New(PtrRange<wchar_t> R) {
  __hc_invariant(!R.isEmpty());
  return Win64UnicodeString::New(R.data(), R.size());
}

bool Win64UnicodeString::isEqual(const Win64UnicodeString& rhs) const {
  if __expect_false(!this->buffer || !rhs.buffer)
    return false;
  if (this->size != rhs.size) 
    return false;
  int ret = __wstrncmp(this->buffer, rhs.buffer, this->getSize());
  return (ret == 0);
}

// "Mutators"

com::PtrRange<wchar_t> Win64UnicodeString::intoRange() const {
  const int sz = getSize();
  if __expect_false(sz == 0)
    return {};
  __hc_invariant(buffer != nullptr);
  return {buffer, buffer + sz};
}

const wchar_t& Win64UnicodeString::frontSafe() const {
  if __expect_false(getSize() == 0)
    return __gnull;
  __hc_invariant(buffer != nullptr);
  return buffer[0];
}

const wchar_t& Win64UnicodeString::backSafe() const {
  if __expect_false(getSize() == 0)
    return __gnull;
  __hc_invariant(buffer != nullptr);
  return buffer[getSize() - 1];
}
