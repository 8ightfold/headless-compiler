//===- Common/EnumArray.hpp -----------------------------------------===//
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
//  An object which can be indexed using enums. Use when enum values
//  are relatively contiguous, otherwise you get a lot of empty slots.
//
//===----------------------------------------------------------------===//

#pragma once

#include "Fundamental.hpp"
#include "PtrRange.hpp"

namespace hc::common {
  template <typename T, typename E,
    E MaxValue = E::MaxValue,
    typename UType = meta::UnderlyingType<E>,
    UType N = static_cast<UType>(MaxValue)>
  struct EnumArray {
    using Type = T;
    using SelfType = EnumArray;
    using UnderlyingType = UType;
    using Enumeration = E;
    static constexpr UType __size = N;
    static constexpr UType __true_size = bool(N) ? N : 1;
  public:
    constexpr T& operator[](Enumeration idx) {
      const auto IX = static_cast<UType>(idx);
      __hc_invariant(IX >= 0 && IX < N);
      return this->__data[IX];
    }

    constexpr const T& operator[](Enumeration idx) const {
      const auto IX = static_cast<UType>(idx);
      __hc_invariant(IX >= 0 && IX < N);
      return this->__data[IX];
    }

    //==================================================================//
    // Observers
    //==================================================================//

    [[nodiscard, gnu::const]]
    constexpr T* data() __noexcept {
      return this->__data;
    }

    [[nodiscard, gnu::const]]
    constexpr const T* data() const __noexcept {
      return this->__data;
    }

    static constexpr UType Size() __noexcept {
      return __size;
    }

    static constexpr UType SizeInBytes() __noexcept {
      return __size * __sizeof(T);
    }

    [[nodiscard]]
    PtrRange<T> intoRange() __noexcept {
      return { begin(), end() };
    }

    [[nodiscard]]
    ImmPtrRange<T> intoRange() const __noexcept {
      return { begin(), end() };
    }

    [[nodiscard, gnu::const]]
    constexpr T* begin() __noexcept {
      return data();
    }

    [[nodiscard, gnu::const]]
    constexpr T* end() __noexcept {
      return data() + Size();
    }

    [[nodiscard, gnu::const]]
    constexpr const T* begin() const __noexcept {
      return data();
    }

    [[nodiscard, gnu::const]]
    constexpr const T* end() const __noexcept {
      return data() + Size();
    }

    static constexpr bool IsEmpty() __noexcept {
      return Size() == 0;
    }

  public:
    T __data[N] { };
  };
} // namespace hc::common
