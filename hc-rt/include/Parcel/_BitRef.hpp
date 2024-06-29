//===- Parcel/_BitRef.hpp -------------------------------------------===//
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
} // namespace hc::parcel
