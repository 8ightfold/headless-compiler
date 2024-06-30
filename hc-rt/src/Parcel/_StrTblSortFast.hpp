//===- Parcel/_StrTblSortFast.hpp -----------------------------------===//
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
#error Incomplete implementation!
// TODO: Finish this eventually ig...

#include <Parcel/StringTable2.hpp>
#include <Common/FastMath.hpp>

namespace hc::parcel {
namespace {
struct ISTableSorter {
  using Arr  = com::PtrRange<_STIdxType>;
  using Iter = _STIdxType*;
  enum { __maxDepthFactor = 2 };
public:
  ISTableSorter(
    IStringTable::BufferType* buf,
    IStringTable::TableType*  tbl) :
   buf(buf->into<com::StrRef>()),
   max_depth(com::bit_log2(tbl->size()) * __maxDepthFactor) {}
public:
  bool do_sort(com::PtrRange<_STIdxType> A) {
    this->introsort(A.begin(), A.end(), max_depth);
    return this->mutated;
  }

protected:
  /// Based on the libcxx `__introsort`.
  void introsort(Iter I, Iter E, usize depth, bool left = true);

  /// @return The number of swaps.
  unsigned branching_sort3(Iter X, Iter Y, Iter Z) {
    if (!this->__comp(*Y, *X)) {
      if (!this->__comp(*Z, *Y))
        return 0;

      ISTableSorter::__swap(Y, Z);
      if (this->__comp(*Y, *X)) {
        ISTableSorter::__swap(X, Y);
        return 2;
      }
      return 1;
    }

    if (this->__comp(*Z, *Y)) {
      ISTableSorter::__swap(X, Z);
      return 1;
    }

    ISTableSorter::__swap(X, Y);
    if (this->__comp(*Z, *Y)) {
      ISTableSorter::__swap(Y, Z);
      return 2;
    }

    return 1;
  }

  void sort3(Iter X, Iter Y, Iter Z) {
    __comp_swap(Y, Z);
    __psort_swap(X, Y, Z);
  }

  void sort4(Iter W, Iter X, Iter Y, Iter Z) {
    __comp_swap(W, Y);
    __comp_swap(X, Z);
    __comp_swap(W, X);
    __comp_swap(Y, Z);
    __comp_swap(X, Y);
  }

  void sort5(Iter V, Iter W, Iter X, Iter Y, Iter Z) {
    __comp_swap(V, W);
    __comp_swap(Y, Z);
    __psort_swap(X, Y, Z);
    __comp_swap(W, Z);
    __psort_swap(V, X, Y);
    __psort_swap(W, X, Y);
  }

private:
  static void __swap(Iter lhs, Iter rhs) {
    const _STIdxType tmp = *rhs;
    *rhs = *lhs;
    *lhs = tmp;
  }

  /// Does ``*lhs < *rhs``.
  bool __comp(_STIdxType lhs, _STIdxType rhs) {
    const usize len = lhs.length;
    if (len < rhs.length)
      return true;
    if (rhs.length > len)
      return false;
    
    // Now for the real lex comp...
    const char* const data = buf.data();
    const char* plhs = data + lhs.offset;
    const char* prhs = data + rhs.offset;

    for (usize Ix = 0; Ix < len; ++Ix) {
      if (plhs[Ix] != prhs[Ix])
        return (plhs[Ix] < prhs[Ix]);
    }

    return false;
  }

  /// Does ``*lhs < *rhs``.
  __always_inline bool __comp(Iter lhs, Iter rhs) {
    return ISTableSorter::__comp(*lhs, *rhs);
  }

  void __comp_swap(Iter X, Iter Y) {
    const bool R = ISTableSorter::__comp(*X, *Y);
    const _STIdxType tmp = R ? *X : *Y;
    *Y = R ? *Y : *X;
    *X = tmp;
    this->mutated |= R;
  }

  void __psort_swap(Iter X, Iter Y, Iter Z) {
    bool R         = ISTableSorter::__comp(*Z, *X);
    const _STIdxType tmp = R ? *Z : *X;
    *Z             = R ? *X : *Z;
    this->mutated |= R;
    R              = ISTableSorter::__comp(tmp, *Y);
    *X             = R ? *X : *Y;
    *Y             = R ? *Y : tmp;
    this->mutated |= R;
  }

private:
  com::StrRef buf;
  bool mutated = false;
  // Default arg
  usize max_depth = 0;
};

void ISTableSorter::introsort(Iter I, Iter E, usize depth, bool left) {
  constexpr usize inssort_upper = 24;
  constexpr usize tuckey_lower  = 128;

  // Handle cases < 5
  while (true) {
    const usize len = (E - I);
    switch (len) {
     case 0:
     case 1:
      return;
     case 2:
      if (this->__comp(--E, I)) {
        ISTableSorter::__swap(I, E);
        this->mutated = true;
      }
      return;
     case 3:
      this->sort3(I, I + 1, --E);
      return;
     case 4:
      this->sort4(I, I + 1, I + 2, --E);
      return;
     case 5:
      this->sort5(I, I + 1, I + 2, I + 3, --E);
      return;
    }

    // Use insertion sort when under threshold.
    if (len < inssort_upper) {
      __hc_todo("introsort::insertion_sort");
      if (left) {
        // this->insertion_sort(I, E);
      } else {
        // this->insertion_sort_ung(I, E);
      }
      return;
    }

    if (depth == 0) {
      __hc_todo("introsort::heap_sort");
      // this->heap_sort(I, E);
      return;
    }
    --depth;

    // Use Tuckey ninther median if over threshold.
    const usize mid = len / 2;
    if (len > tuckey_lower) {
      this->mutated = true;
      this->branching_sort3(I, I + mid, E - 1);
      this->branching_sort3(I + 1, I + (mid - 1), E - 2);
      this->branching_sort3(I + 2, I + (mid + 1), E - 3);
      this->branching_sort3(I + (mid - 1), I + mid, I + (mid + 1));
      ISTableSorter::__swap(I, I + mid);
    } else {
      const unsigned swaps =
        this->branching_sort3(I + mid, I, E - 1);
      this->mutated |= !!swaps;
    }

    if (!left && __comp(I - 1, I)) {

    }

     __hc_todo("introsort");
  }
}
} // namespace `anonymous`
} // namespace hc::parcel
