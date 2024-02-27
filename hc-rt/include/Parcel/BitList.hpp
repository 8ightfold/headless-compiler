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
      for (usize I = 0; I < __count; ++I)
        __data[I] = BType(0);
      return *this;
    }

    inline constexpr bool flip(usize I) {
      __hc_invariant(I < Size());
      const BType V = Off(I);
      return (__data[Idx(I)] ^= V) & V;
    }

    inline constexpr bool get(usize I) const {
      __hc_invariant(I < Size());
      return __data[Idx(I)] & Off(I);
    }

    inline constexpr void set(usize I) {
      __hc_invariant(I < Size());
      __data[Idx(I)] |= Off(I);
    }

    constexpr usize accumulateCount() const {
      usize total = 0;
      for (BitListType I : __data)
        total += popcnt(I);
      return total;
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
  using ALBitList = BitList<CC::Align::UpEq(N)>;
} // namespace hc::parcel
