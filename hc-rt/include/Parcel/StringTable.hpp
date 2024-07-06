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
  struct [[gnu::packed, clang::trivial_abi]] StrTblIdx {
    u16 offset = 0;
    u16 length = 0;
  };

  /// Base for string tables. Uses the buffer as a bump allocator, 
  /// which means generally strings cannot be removed once added.
  /// The main exception is `pop_back`, which is allowed if not dirty.
  /// On the other hand, table elements *can* be sorted and removed.
  struct [[gsl::Pointer]] IStringTable {
    using SelfType   = IStringTable;
    using BufferType = IStaticVec<char>;
    using TableType  = IStaticVec<StrTblIdx>;

    struct Iterator {
      using difference_type = uptrdiff;
      using value_type = com::StrRef;
    public:
      bool operator==(const Iterator&) const = default;
      value_type operator*() const { return this->resolve(); }
      // TODO: Add ValuePtrProxy<...>
      // value_type operator->() const { return __iter_val; }

      Iterator& operator++();
      Iterator operator++(int) {
        Iterator tmp = *this;
        ++*this;
        return tmp;
      }
    
    private:
      value_type resolve() const;
    public:
      const IStringTable* __base  = nullptr;
      const StrTblIdx* __iter_val = nullptr;
    };

    struct DataFlags {
      bool null_term  : 1; // Strings should be null-terminated.
      bool dirty      : 1; // Strings are out of order.
      bool ksorted    : 1; // Strings are to be kept sorted.
      bool is_sorted  : 1; // `true` when still known to be sorted.
      bool imp_empty  : 1; // If empty values should be implicit.
      bool has_empty  : 1; // If the table "contains" the empty string.
    };

    enum class Status {
      atCapacity    = -1, // The capacity of an array would be exceeded.
      success       = 0,  // Insertion success.
      alreadyExists = 1,  // Only used when (ksorted == true) or if empty.
    };

    /// @brief The case for invalid insertion.
    static constexpr auto invalidString 
      = com::StrRef::New(nullptr, usize(0));

  public:
    constexpr IStringTable(
      BufferType& buf, TableType& tbl) :
     buf(&buf), tbl(&tbl) {
      this->flags.is_sorted = true;
      // TODO: Remove when complete.
      this->flags.imp_empty = true;
    }
  
  public:
    /// Attempts to add a new string to the table. Normally this
    /// will always append the string, even if it already exists.
    /// But when `flags::ksorted` is true, it will do a binary search to
    /// see if the element already exists. If not, it will be appended.
    /// @return A pair with a span over the inserted string or the
    /// existing string, and a `Status`.
    com::Pair<com::StrRef, Status> insert(com::StrRef S);

    /// Attempts to pop the last string from the table. This can only
    /// be done if the table is both unsorted and clean.
    /// @return `true` if succeeds.
    bool pop();

    //==================================================================//
    // Settings
    //==================================================================//

    /// Sets the null terminator flag if buffer is inactive.
    /// @return The the current value of `flags::null_term`.
    bool setNullTerminationPolicy(bool V) {
      if __expect_false(this->isBufferInUse()) {
        // TODO: Output a warning.
        return flags.null_term;
      }
      this->flags.null_term = V;
      return V;
    }

    /// Sets the keep sorted flag. Will not unsort when false.
    void setKSortPolicy(bool V);

    // TODO: Add setImplicitEmptyValuePolicy, sometimes we might want em.
    //  Also add setDestructivePopPolicy for popping sorted elements.

    //==================================================================//
    // Mutators
    //==================================================================//

    /// Sorts strings in lexicographic order, in shortlex form.
    /// This is *usually* done using the introsort algorithm (TODO). 
    /// Sorting will set the dirty bit, which can be undone using `unsort`.
    /// @param keep_sorted Whether to sort newly inserted elements.
    void shortlexSort(bool keep_sorted = false);

    /// Reverts to insertion order, unsets dirty bit.
    /// It also sets `ksorted` to false.
    void unsort();

    SelfType& erase() __noexcept {
      this->clear();
      return *this;
    }

    void clear() __noexcept {
      buf->clear();
      tbl->clear();
      // Unset flags
      flags.dirty = false;
      flags.is_sorted = true;
      flags.has_empty = false;
    }

    //==================================================================//
    // Observers
    //==================================================================//

    Iterator begin() const {
      if (!flags.has_empty)
        $tail_return this->ibegin();
      else
        // Return a fake value when empty value must appear.
        return {this, nullptr};
    }
    Iterator ibegin() const { return {this, tbl->begin()}; }
    Iterator end()    const { return {this, tbl->end()}; }

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

    /// @return `true` if strings aren't out of order.
    bool isClean() const { return !flags.dirty; }

    /// @tparam Permissive Considers a table with under two elements sorted.
    /// @return `true` if strings are in order and sorted.
    template <bool Permissive = false>
    bool isSorted() const {
      if constexpr (Permissive)
        return flags.is_sorted || (tbl->size() < 2);
      else
        return flags.is_sorted;
    }

    bool isPoppable() const {
      return this->isBufferInUse()
        && !this->isSorted()
        && !this->isDirty();
    }

  protected:
    /// Returns a `StrRef` from an index.
    com::StrRef resolveDirect(StrTblIdx I) const;
    /// Returns a `StrRef` from an index table index.
    com::StrRef resolveAt(usize Ix) const;

    /// Returns the associated string in the table for `S`, if found.
    /// If permissive `isSorted` is `false`, or the string is not found,
    /// `invalidString` will be returned.
    com::StrRef locateString(com::StrRef S) const;

    /// Returns an empty string for the current null termination strategy.
    inline com::StrRef getEmptyString() const {
      static constexpr auto on  = com::StrRef::New("", 1);
      static constexpr auto off = com::StrRef::New("", usize(0));
      return (flags.null_term) ? on : off;
    }

    /// Checks if the table has storage for the requested string.
    /// Assumes `.dropNull()` has been called.
    /// @return If the input string can be inserted.
    bool doesHaveStorageFor(com::StrRef S) const;
  
  private:
    /// Adds a string to the back of the table.
    /// Assumes `.dropNull()` has been called.
    com::StrRef appendDirect(com::StrRef S);

    /// Adds a string to the back of the table.
    /// Assumes `.dropNull()` has been called.
    StrTblIdx appendDirectIter(com::StrRef S);

    /// Inserts a string into a sorted array without adding index.
    /// Assumes `.dropNull()` has been called.
    com::Pair<com::StrRef, Status> binaryInsert(com::StrRef S);

    /// Internal binary search algo. Same rules as `locateString`.
    com::StrRef binarySearch(com::StrRef S) const;

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
    StaticVec<StrTblIdx, TableSize> __tbl;
  };

} // namespace hc::parcel
