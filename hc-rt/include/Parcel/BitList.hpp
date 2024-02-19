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

#pragma once

#include "Common.hpp"

namespace hc::parcel {
  using BitListType = u64;

  template <meta::is_integral Int>
  __always_inline constexpr
   Int popcnt(Int I) noexcept {
    if constexpr (sizeof(I) == 8)
      return __builtin_popcountll(I);
    else
      return __builtin_popcount(I);
  }

  template <typename BType = BitListType>
  struct _BitRef {
    static constexpr auto __perIx = __bitsizeof(BType);
  public:
    constexpr _BitRef(BType& loc, uptr off)
     : __loc(loc), __off(uptr(1U) << off) { }
    
    constexpr _BitRef& operator=(bool B) {
      this->doMask(B);
      return *this;
    }

    explicit constexpr operator bool() const {
      return bool(__loc & __off);
    }

    __always_inline constexpr void doMask(bool B) {
      B ? __doMask<true>() : __doMask<false>();
    }
  
    template <bool B>
    constexpr void __doMask() {
      __loc |= __off;
    }

    template <>
    constexpr void __doMask<false>() {
      __loc &= ~__off;
    }

  public:
    BType& __loc;
    const uptr __off;
  };

  template <usize N, typename BType = BitListType>
  struct BitList {
    using SelfType = BitList;
    static constexpr auto __perIx = __bitsizeof(BType);
    static constexpr auto __count = (N / __perIx) + 1;
    /// The total allotted bits.
    static constexpr usize bitCount = __count * __perIx;
  public:
    constexpr _BitRef<BType> operator[](usize I) {
      __hc_invariant(I < Size());
      return {__data[Idx(I)], I % __perIx};
    }

    constexpr BitList& reset() {
      return (*this = BitList{});
    }

    constexpr bool flip(usize I) {
      __hc_invariant(I < Size());
      const BType V = Off(I);
      return (__data[Idx(I)] ^= V) & V;
    }

    constexpr bool get(usize I) const {
      __hc_invariant(I < Size());
      return __data[Idx(I)] & Off(I);
    }

    constexpr usize accumulateCount() const {
      usize total = 0;
      for (BitListType I : __data)
        total += popcnt(I);
      return total;
    }

    static constexpr usize Size() { return N; }
    static constexpr usize Bits() { return N; }

    constexpr auto __UData()
     -> BitListType(&)[__count] {
      return this->__data; 
    }

    __always_inline static constexpr usize Idx(usize I) {
      return I / __perIx;
    }
    __always_inline static constexpr usize Off(usize I) {
      return usize(1) << (I % __perIx);
    }

  public:
    BitListType __data[__count] { };
  };
} // namespace hc::parcel
