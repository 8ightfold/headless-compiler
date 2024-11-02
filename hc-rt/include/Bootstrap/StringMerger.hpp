//===- Bootstrap/StringMerger.hpp -----------------------------------===//
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

#pragma once

#include "UnicodeString.hpp"

namespace hc::bootstrap {

struct IUStringMerger {
  IUStringMerger(wchar_t* buf, usize N) :
   buf(buf), capacity(N) {
  }

  IUStringMerger& merge(auto&&...VV) {
    ((this->add(VV)), ...);
    return *this;
  }

  IUStringMerger& add(const UnicodeString& US);
  IUStringMerger& add(const wchar_t* S);
  IUStringMerger& add(const wchar_t* S, usize n);

  UnicodeString toUStr() const {
    return UnicodeString::New(buf, capacity);
  }

  bool checkCap(usize n) const;

private:
  wchar_t* buf = nullptr;
  u32 len = 0;
  u32 capacity = 0;
};

template <usize N>
struct UStringMerger : public IUStringMerger {
  UStringMerger() : IUStringMerger(__buf, N) {}
private:
  wchar_t __buf[N] {};
};

} // namespace hc::bootstrap
