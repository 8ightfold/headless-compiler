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

#include <Common/Pair.hpp>
#include <Common/StrRef.hpp>
#include <Parcel/StaticVec.hpp>

namespace hc::parcel {
  /// Stores strings as `[Offset, Length]`.
  struct [[gnu::packed]] _STIdxType {
    u16 offset = 0;
    u16 length = 0;
  };

  /// Base for string tables. Uses the buffer as a bump allocator, 
  /// which means generally strings cannot be removed once added.
  /// On the other hand, table elements *can* be sorted and removed.
  struct [[gsl::Pointer]] IStringTable {
    using SelfType   = IStringTable;
    using BufferType = IStaticVec<char>;
    using TableType  = IStaticVec<_STIdxType>;

    struct DataFlags {
      bool null_term  : 1; /// Strings should be null-terminated.
      bool dirty      : 1; /// Strings are out of order.
      bool ksorted    : 1; /// Strings are to be kept sorted.
    };

  public:
    constexpr IStringTable(
      BufferType& buf, TableType& tbl) :
     buf(&buf), tbl(&tbl) {}
  
  public:
    /// Sorts strings in lexicographic order, in shortlex form.
    /// This is done using the introsort algorithm. Sorting will 
    /// set the dirty bit, which can be undone using `unsort`.
    /// @param keep_sorted Whether to sort newly inserted elements.
    void sortLexicographically(bool keep_sorted = false);

    /// Reverts to insertion order, unsets dirty bit.
    void unsort();

    //==================================================================//
    // Mutators
    //==================================================================//

    SelfType& erase() __noexcept {
      this->clear();
      return *this;
    }

    void clear() __noexcept {
      buf->clear();
      tbl->clear();
    }

    //==================================================================//
    // Observers
    //==================================================================//

    usize size() const { return tbl->size(); }
    usize sizeInBytes() const { return buf->size(); }

    usize capacity() const { return tbl->capacity(); }
    usize capacityInBytes() const { return buf->capacity(); }
    
    /// @brief Used for checking if we can modify flags.
    /// @return `true` if the buffer has already been used. 
    bool isBufferInUse() const { return !buf->isEmpty(); }

    /// @return `true` if strings are null-terminated.
    bool isNullTerminated() const { return flags.null_term; }

    /// Same as `isNullTerminated`.
    bool isCStrs() const { return flags.null_term; }

    /// @return `true` if strings are out of order.
    bool isDirty() const { return flags.dirty; }

  protected:
    BufferType* buf  = nullptr;
    TableType*  tbl  = nullptr;
    DataFlags flags  = {};
  };

  //====================================================================//
  // Table Implementation
  //====================================================================//

  template <usize BufferSize, usize TableSize>
  struct [[gsl::Owner]] StringTable : IStringTable {
    using BaseType = IStringTable;
    using SelfType = StringTable;
  public:
    constexpr StringTable() :
     IStringTable(__buf, __tbl) {}
    
    /// Disable copying and moving.
    HC_MARK_DELETED(StringTable);
    
    BaseType* operator->() {
      return static_cast<BaseType*>(this);
    }
    const BaseType* operator->() const {
      return static_cast<const BaseType*>(this);
    }

  private:
    StaticVec<char, BufferSize>      __buf;
    StaticVec<_STIdxType, TableSize> __tbl;
  };

} // namespace hc::parcel
