//===- Bootstrap/StringMerger.cpp -----------------------------------===//
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

#include <Bootstrap/StringMerger.hpp>
#include <Common/InlineMemcpy.hpp>
#include <Common/InlineMemset.hpp>
#include <Common/Strings.hpp>

using namespace hc;
using namespace hc::bootstrap;

IUStringMerger& IUStringMerger::add(const UnicodeString& US) {
  return this->add(US.buffer, US.getSize());
}

IUStringMerger& IUStringMerger::add(const wchar_t* S) {
  if __expect_false(!S)
    return *this;
  const usize n = __wstrlen(S);
  return this->add(S, n);
}

IUStringMerger& IUStringMerger::add(const wchar_t* S, usize n) {
  if __expect_false(!S || !n)
    return *this;
  if __likely_false(!this->checkCap(n))
    return *this;
  com::inline_memcpy(this->buf + len, S, n * sizeof(wchar_t));
  this->len += n;
  return *this;
}

IUStringMerger& IUStringMerger::reset() {
  if (!buf || !capacity)
    return *this;
  com::inline_memset(buf, 0, this->len * sizeof(wchar_t));
  this->len = 0;
  return *this;
}

bool IUStringMerger::checkCap(usize n) const {
  return (this->len + n) < this->capacity;
}
