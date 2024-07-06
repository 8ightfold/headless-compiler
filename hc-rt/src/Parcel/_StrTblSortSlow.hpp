//===- Parcel/_StrTblSortSlow.hpp -----------------------------------===//
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

#include <Parcel/StringTable.hpp>
#include <Common/FastMath.hpp>

namespace hc::parcel {
namespace {
struct ISTableSorter {
  using Arr  = com::PtrRange<StrTblIdx>;
  using Iter = StrTblIdx*;
public:
  ISTableSorter(
    IStringTable::BufferType* buf,
    IStringTable::TableType*) :
   buf(buf->into<com::StrRef>()) {}
public:
  bool do_sort(com::PtrRange<StrTblIdx> A) {
    this->insertion_sort(A.begin(), A.end());
    return this->mutated;
  }

protected:
  void insertion_sort(Iter I, Iter E);

private:
  static void __swap(Iter lhs, Iter rhs) {
    const StrTblIdx tmp = *rhs;
    *rhs = *lhs;
    *lhs = tmp;
  }

  /// Does ``*lhs < *rhs``.
  bool __comp(StrTblIdx lhs, StrTblIdx rhs) {
    const usize len = lhs.length;
    if (len < rhs.length)
      return true;
    if (rhs.length > len)
      return false;
    if (len == 0)
      return false;
    
    // Now for the real lex comp...
    const char* const data = buf.data();
    const char* plhs = data + lhs.offset;
    const char* prhs = data + rhs.offset;

    return com::__strncmp(plhs, prhs, len) < 0;
  }

  /// Does ``*lhs < *rhs``.
  __always_inline bool __comp(Iter lhs, Iter rhs) {
    return this->__comp(*lhs, *rhs);
  }

private:
  com::StrRef buf;
  bool mutated = false;
};

void ISTableSorter::insertion_sort(Iter I, Iter E) {
  const usize len = (E - I);
  switch (len) {
   case 0:
   case 1:
    return;
   case 2:
    if (__comp(--E, I)) {
      ISTableSorter::__swap(I, E);
      this->mutated = true;
    }
    return;
  }

  Iter A = I;
  for (usize Ix = 1; Ix < len; ++Ix) {
    for (usize J = Ix; (J > 0) && __comp(A[J], A[J - 1]); --J) {
      ISTableSorter::__swap(A + (J - 1), A + J);
      this->mutated = true;
    }
  }
}
} // namespace `anonymous`
} // namespace hc::parcel
