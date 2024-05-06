//===- Parcel/StringTable.hpp ---------------------------------------===//
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

#pragma once

#include <Common/StrRef.hpp>
#include <Common/Strings.hpp>
#include <Parcel/StaticVec.hpp>

namespace hc::parcel {
  /// @brief Base for an intern table. Allows for inline storage.
  /// @tparam Char The character type of the stored strings.
  /// @tparam BufferSize The size of the inlined buffer.
  template <CC::__xchar_type Char, usize BufferSize>
  struct [[gsl::Owner]] IStringTable :
   protected StaticVec<Char, BufferSize> {
    using RangeType = CC::ImmPtrRange<Char>;
    using StrType  = const Char*;
    using BaseType = StaticVec<Char, BufferSize>;
    using SelfType = IStringTable;
  public:
    /// Default constructor.
    constexpr IStringTable() : BaseType() {}
    
    RangeType append(StrType S, bool permissive = false) {
      if __expect_false(!S) {
        __hc_assert(permissive);
        return { };
      }
      const usize len = stringlen(S);
      return RangeType::New(S, len);
    }
  };

  //====================================================================//
  // Table Implementation
  //====================================================================//

  template <CC::__xchar_type Char, usize N, usize BufferSize>
  struct TStringTable : protected IStringTable<Char, BufferSize> {
    using RangeType = CC::ImmPtrRange<Char>;
    using StrType  = const Char*;
    using BaseType = IStringTable<Char, BufferSize>;
    using SelfType = TStringTable;
    // Storage Types
    using StoType = StaticVec<RangeType, N>;
    using IStoType = IStaticVec<RangeType>;
    // Info Values
    static constexpr usize npos = Max<usize>;
  public:
    /// Default constructor.
    constexpr TStringTable() : BaseType(), __elems() {}
  
    RangeType append(StrType S, bool permissive = false) {
      const RangeType out = this->append(S, permissive);
      if (auto O = __elems.emplaceBack(out); __expect_true(O))
        return *O;
      return { };
    }

    inline usize findPartial(RangeType S) const {
      $tail_return this->__find<false>(S);
    }

    inline usize findExact(RangeType S) const {
      $tail_return this->__find<true>(S);
    }

    usize find(RangeType S, bool partial = false) const {
      if (partial)
        return this->findPartial(S);
      else
        return this->findExact(S);
    }

    /// Finds a matching string in the table.
    /// @returns `npos` if not found.
    usize find(StrType S, bool partial = false) const {
      if __expect_false(!S)
        return npos;
      const auto len = stringlen(S);
      RangeType R {S, S + len};
      return this->find(R, partial);
    }

    RangeType* begin() { return __elems.begin(); }
    RangeType* end() { return __elems.begin(); }
    const RangeType* begin() const { return __elems.begin(); }
    const RangeType* end() const { return __elems.begin(); }

    IStoType* operator->() { return &__elems; }
    const IStoType* operator->() const { return &__elems; }
  
  protected:
    template <bool IsExact>
    usize __find(RangeType S) const {
      usize pos = Max<usize>;
      for (RangeType R : this->__elems) {
        ++pos;
        if constexpr (IsExact) {
          if (R.size() != S.size())
            continue;
        } else {
          if (R.size() < S.size())
            continue;
        }
        const int cmp = CC::__memcmp(
          S.data(), R.data(), S.sizeInBytes());
        if (cmp == 0)
          return pos;
      }
      return npos;
    }

  protected:
    StoType __elems;
  };

  template <usize N, usize BufferSize>
  using StringTable = TStringTable<char, N, BufferSize>;

  template <usize N, usize BufferSize>
  using WStringTable = TStringTable<wchar_t, N, BufferSize>;
} // namespace hc::parcel
