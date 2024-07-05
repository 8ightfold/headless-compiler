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
#include <Common/InlineMemcpy.hpp>
#include <Common/Strings.hpp>

#if _HC_FAST_STRING_TABLE
# include "_StrTblSortFast.hpp"
#else
# include "_StrTblSortSlow.hpp"
#endif

using namespace hc;
using namespace hc::parcel;
using Status = IStringTable::Status;

com::Pair<com::StrRef, Status> IStringTable::insert(com::StrRef S) {
  S.dropNullMut();
  // Empty strings always return.
  if (S.isEmpty()) {
    this->flags.has_empty = true;
    return {this->getEmptyString(), Status::alreadyExists};
  }

  // Handle cases where there is no capacity.
  if __likely_false(!this->doesHaveStorageFor(S)) {
    if (!this->isSorted<true>()) {
      // If we don't have immediate storage for the string, and we aren't
      // sorting the strings, immediately return.
      return {invalidString, Status::atCapacity};
    }
    // Gets the string currently in the array (if it exists).
    const com::StrRef curr = this->binarySearch(S);
    if (invalidString.isEqual(curr))
      return {invalidString, Status::atCapacity};
    return {curr, Status::alreadyExists};
  }

  if (!flags.ksorted) {
    // Mark unsorted if inserting. Does not affect dirtiness.
    this->flags.is_sorted = false;
    return {this->appendDirect(S), Status::success};
  }

  // Do some fancy-shmancy stuff to insert sorted.
  return this->binaryInsert(S);
}

//======================================================================//
// Mutators
//======================================================================//

void IStringTable::sortLexicographically(bool keep_sorted) {
  // TODO: At some point add runtime dispatch for checking the dirty bit.
  // We should probably check if ksorted is true, as that should probably
  // enable the dirty bit no matter what... not sure though.
  this->flags.ksorted = keep_sorted;
  this->flags.is_sorted = true;
  ISTableSorter S(this->buf, this->tbl);
  this->flags.dirty = S.do_sort(tbl->intoRange());
}

void IStringTable::unsort() {
  this->flags.ksorted = false;
  this->flags.is_sorted = false;
  /// Do nothing if never dirtied.
  if (this->isClean())
    return;

#if _HC_FAST_STRING_TABLE
  __hc_todo("unsort");
#else
  const usize len = this->size();
  StrTblIdx* A = tbl->data();
  for (usize Ix = 0; Ix < len; ++Ix) {
    const StrTblIdx tmp = A[Ix];
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

//======================================================================//
// Internals
//======================================================================//

com::StrRef IStringTable::resolveDirect(StrTblIdx I) const {
  __hc_invariant(I.offset + I.length <= buf->size());
  auto* const P = buf->data() + usize(I.offset);
  return com::StrRef::New(P, I.length);
}

com::StrRef IStringTable::resolveAt(usize Ix) const {
  __hc_invariant(Ix < tbl->size())
  const StrTblIdx I = (*tbl)[Ix];
  return this->resolveDirect(I);
}

com::StrRef IStringTable::locateString(com::StrRef S) const {
  if __expect_false(!this->isSorted<true>())
    return invalidString;
  // Handle empty strings.
  if (S.isEmpty()) {
    if (flags.has_empty)
      return this->getEmptyString();
    return invalidString;
  }
  return this->binarySearch(S);
}

bool IStringTable::doesHaveStorageFor(com::StrRef S) const {
  /// Always available.
  if (S.isEmpty())
    return true;
  if (tbl->isFull())
    return false;
  const usize len = (S.size() + flags.null_term);
  return (buf->__remainingCapacity() >= len);
}

com::StrRef IStringTable::appendDirect(com::StrRef S) {
  __hc_assert(!tbl->isFull());
  const auto [off, len] = this->appendDirectIter(S);
  // Add the new table index.
  tbl->emplace(off, len);
  return com::StrRef::New(buf->data() + off, usize(len));
}

StrTblIdx IStringTable::appendDirectIter(com::StrRef S) {
  const usize len = S.size();
  // Resize with enough space for the null terminator if required.
  char* const ptr = buf->growUninit(len + flags.null_term);
  com::inline_memcpy(ptr, S.data(), len);
  // Set the null terminator.
  if (flags.null_term)
    ptr[len] = '\0';
  // Add the new table index.
  return {u16(ptr - buf->begin()), u16(len)};
}

com::Pair<com::StrRef, Status> IStringTable::binaryInsert(com::StrRef S) {
  // Empty strings are not allowed, table MUST be sorted.
  __hc_invariant(!S.isEmpty() && this->isSorted<true>());
  if __expect_false(tbl->isEmpty())
    return {this->appendDirect(S), Status::success};
  
  // Check for the simple append case.
  $scope {
    auto last = this->resolveDirect(tbl->back());
    if (last.size() > S.size())
      break;
    if (S.size() > last.size()) {
      // S is larger than last here, so we do a simple append.
      return {this->appendDirect(S), Status::success};
    }
    const int R = com::__strncmp(
      S.data(), last.data(), last.size());
    if (R == 0)
      return {last, Status::alreadyExists};
  }

  const usize len = S.size();
  auto* const ptbl = tbl->data();
  usize lhs = 0;
  usize rhs = tbl->size();

  while (lhs < rhs) {
    const usize mid = (lhs + rhs) >> 1;
    if (const auto ilen = ptbl[mid].length; ilen != len) {
      if (len < ilen)
        rhs = mid - 1;
      else if (len > ilen)
        lhs = mid + 1;
      continue;
    }

    // tblS.size() MUST be equal to S.size() here.
    const auto tblS = this->resolveDirect(ptbl[mid]);
    // Compare the actual string contents.
    const int R = com::__strncmp(S.data(), tblS.data(), len);
    if (R == 0)
      // S == tbl[mid]
      return {tblS, Status::alreadyExists};
    else if (R < 0)
      // S < tbl[mid]
      rhs = mid - 1;
    else
      // S > tbl[mid]
      lhs = mid + 1;
  }

  const usize insert_pos = lhs;
  // We already checked the back so it should be fine,
  // but add a sanity check just in case.
  __hc_invariant(insert_pos < tbl->size());
  if (const auto tblS = this->resolveAt(insert_pos); tblS.isEqual(S)) {
    // Return the string at this position if it exists.
    return {tblS, Status::alreadyExists};
  }

  // Another one to make sure we can insert the string.
  __hc_invariant(this->doesHaveStorageFor(S));
  // Begin slide copying.
  constexpr usize slide_len = 8;
  StrTblIdx sbuf[slide_len];
  auto* first = tbl->data() + insert_pos;
  auto* last  = tbl->growUninit(); // Old ::end().

  // Copies in blocks until too small for buffer.
  auto run_slide = [&] -> bool {
    // Under the current rules this should never fail, but ya never know...
    __hc_invariant((last >= first));
    if ((last - first) < slide_len)
      return false;
    auto* slide = last - slide_len;
    // TODO: Implement memmove so I can remove this...
    static constexpr usize cpy_len 
      = slide_len * sizeof(StrTblIdx);
    com::inline_memcpy(sbuf, slide, cpy_len);
    com::inline_memcpy(slide + 1, sbuf, cpy_len);
    last = slide - 1;
    return true;
  };

  while (run_slide()) {}
  // If dist(first, last) > 0.
  if (const usize part_slide_len = (last - first)) {
    const usize cpy_len =
      part_slide_len * sizeof(StrTblIdx);
    com::inline_memcpy(sbuf, first, cpy_len);
    com::inline_memcpy(first + 1, sbuf, cpy_len);
  }

  const auto iter = this->appendDirectIter(S);
  // Add the new table index at the insert position.
  ptbl[insert_pos] = iter;
  const auto new_str = com::StrRef::New(
    buf->data() + iter.offset, usize(len));
  return {new_str, Status::success};
}

com::StrRef IStringTable::binarySearch(com::StrRef S) const {
  // Empty strings are not allowed, table MUST be sorted.
  __hc_invariant(!S.isEmpty() && this->isSorted<true>());
  if __expect_false(tbl->isEmpty())
    return invalidString;
  if __likely_false(tbl->size() == 1) {
    /// Considered sorted.
    const com::StrRef res = this->resolveAt(0);
    if (res.isEqual(S))
      return res;
    return invalidString;
  }
  
  const usize len = S.size();
  auto* const ptbl = tbl->data();
  usize lhs = 0;
  usize rhs = tbl->size() - 1;

  while (lhs <= rhs) {
    const usize mid = (lhs + rhs) >> 1;
    if (const auto ilen = ptbl[mid].length; ilen != len) {
      if (len < ilen)
        rhs = mid - 1;
      else if (len > ilen)
        lhs = mid + 1;
      continue;
    }

    // tblS.size() MUST be equal to S.size() here.
    const auto tblS = this->resolveDirect(ptbl[mid]);
    // Compare the actual string contents.
    const int R = com::__strncmp(S.data(), tblS.data(), len);
    if (R == 0)
      // S == tbl[mid]
      return tblS;
    else if (R < 0)
      // S < tbl[mid]
      rhs = mid - 1;
    else
      // S > tbl[mid]
      lhs = mid + 1;
  }

  return invalidString;
}
