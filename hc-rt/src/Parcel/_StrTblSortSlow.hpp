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

#include <Parcel/StringTable2.hpp>
#include <Common/FastMath.hpp>

using namespace hc;
using namespace hc::parcel;

namespace {
struct ISTableSorter {
  using Arr  = PtrRange<_STIdxType>;
  using Iter = _STIdxType*;
  enum { __maxDepthFactor = 2 };
public:
  ISTableSorter(
    IStringTable::BufferType* buf,
    IStringTable::TableType*  tbl) :
   buf(buf->into<StrRef>()),
   max_depth(com::bit_log2(tbl->size()) * __maxDepthFactor) {}
public:
  bool introsort(PtrRange<_STIdxType> A) {
    this->introsort(A.begin(), A.end(), max_depth);
    return this->mutated;
  }

protected:
  void introsort(Iter I, Iter E, usize depth, bool left = true);

private:
  StrRef buf;
  bool mutated = false;
  // Default arg
  usize max_depth = 0;
};

void ISTableSorter::introsort(Iter I, Iter E, usize depth, bool left) {
  
}
} // namespace `anonymous`
