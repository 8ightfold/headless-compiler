//===- Parcel/StringTable.cpp ---------------------------------------===//
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
//
//  A container which allows for simple creation of statically-backed
//  strings in a dynamic context. Used as the base for intern tables.
//
//===----------------------------------------------------------------===//

#include <Parcel/StringTable2.hpp>
#include <Common/FastMath.hpp>

#if _HC_FAST_STRING_TABLE
# include "_StrTblSortFast.hpp"
#else
# include "_StrTblSortSlow.hpp"
#endif

using namespace hc;
using namespace hc::parcel;

void IStringTable::sortLexicographically(bool keep_sorted) {
  // TODO: At some point add runtime dispatch for checking the dirty bit.
  // We should probably check if ksorted is true, as that should probably
  // enable the dirty bit no matter what... not sure though.
  this->flags.ksorted = keep_sorted;
  ISTableSorter S(this->buf, this->tbl);
  this->flags.dirty = S.do_sort(tbl->intoRange());
}

void IStringTable::unsort() {
  this->flags.ksorted = false;
  /// Do nothing if never dirtied.
  if (this->isClean())
    return;

#if _HC_FAST_STRING_TABLE
  __hc_todo("unsort");
#else
  const usize len = this->size();
  _STIdxType* A = tbl->data();
  for (usize Ix = 0; Ix < len; ++Ix) {
    const _STIdxType tmp = A[Ix];
    usize J = Ix;
    for (; (J > 0) && (A[J - 1].offset > tmp.offset); --J) {
      A[J] = A[J - 1];
    }
    A[J] = tmp;
  }
#endif
  /// Mark as clean.
  this->flags.dirty = false;
}
