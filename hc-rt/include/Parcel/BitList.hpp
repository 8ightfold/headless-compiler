//===- Parcel/BitList.hpp -------------------------------------------===//
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
//  General purpose container for dealing with packed bits. Used for
//  things like BitList and Skiplist.
//
//===----------------------------------------------------------------===//

#pragma once

#include "_BitRef.hpp"
#include <Common/Memory.hpp>

namespace hc::parcel {
  template <usize N, typename BType = BitListType>
  struct BitList {
    using SelfType = BitList;
    static constexpr usize __perIx = __bitsizeof(BType);
    static constexpr usize __count = (N / __perIx) + 1;
    /// The total allotted bits.
    static constexpr usize bitCount = __count * __perIx;
  public:
    constexpr _BitRef<BType> operator[](usize I) {
      __hc_invariant(I < Size());
      return {__data[Idx(I)], I % __perIx};
    }

    inline constexpr BitList& reset() {
      if $is_consteval() {
        for (usize I = 0; I < __count; ++I)
          __data[I] = BType(0);
      } else {
        com::__array_memset(this->__data, 0);
      }

      return *this;
    }

    /// Inverts the state of the bit at `I`,
    /// and then returns the new value.
    inline constexpr bool flip(usize I) {
      __hc_invariant(I < Size());
      const BType V = Off(I);
      return (__data[Idx(I)] ^= V) & V;
    }

    /// Returns the state of the bit at `I`.
    inline constexpr bool get(usize I) const {
      __hc_invariant(I < Size());
      return __data[Idx(I)] & Off(I);
    }

    /// Sets the bit at `I` to `1`.
    inline constexpr void set(usize I) {
      __hc_invariant(I < Size());
      __data[Idx(I)] |= Off(I);
    }

    /// Returns the number of bits set to `1`.
    constexpr usize countActive() const {
      usize total = 0;
      for (BitListType I : __data)
        total += popcnt(I);
      return total;
    }

    /// Returns the number of bits set to `0`.
    inline constexpr usize countInactive() const {
      return __count - this->countActive();
    }

    constexpr auto __uData()
     -> BitListType(&)[__count] {
      return this->__data; 
    }

    static constexpr usize Size() { return N; }
    static constexpr usize __USize() { return __count; }
    
    __always_inline static constexpr usize Idx(usize I) {
      return I / __perIx;
    }
    __always_inline static constexpr usize Off(usize I) {
      return usize(1) << (I % __perIx);
    }

  public:
    BitListType __data[__count] { };
  };

  template <usize N>
  using ALBitList = BitList<com::Align::UpEq(N)>;
} // namespace hc::parcel
