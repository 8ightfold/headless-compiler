//===- Common/Array.hpp ---------------------------------------------===//
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

#include "Fundamental.hpp"
#include "PtrRange.hpp"

namespace hc::common {
  template <typename T, usize N>
  struct Array {
    using Type = T;
    using SelfType = Array<T, N>;
    static constexpr usize __size = N;
  public:
    constexpr T& operator[](usize IX) {
      __hc_invariant(IX < N);
      return this->__data[IX];
    }

    constexpr const T& operator[](usize IX) const {
      __hc_invariant(IX < N);
      return this->__data[IX];
    }

    //=== Observers ===//

    [[nodiscard, gnu::const]]
    constexpr T* data() __noexcept {
      return this->__data;
    }

    [[nodiscard, gnu::const]]
    constexpr const T* data() const __noexcept {
      return this->__data;
    }

    static constexpr usize Size() __noexcept {
      return __size;
    }

    static constexpr usize SizeInBytes() __noexcept {
      return __size * __sizeof(T);
    }

    [[nodiscard]]
    PtrRange<T> toPtrRange() __noexcept {
      return { begin(), end() };
    }

    [[nodiscard]]
    PtrRange<const T> toPtrRange() const __noexcept {
      return { begin(), end() };
    }

    [[nodiscard, gnu::const]]
    constexpr T* begin() __noexcept {
      return data();
    }

    [[nodiscard, gnu::const]]
    constexpr T* begin() const __noexcept {
      return data();
    }

    [[nodiscard, gnu::const]]
    constexpr T* end() __noexcept {
      return data() + Size();
    }

    [[nodiscard, gnu::const]]
    constexpr T* end() const __noexcept {
      return data() + Size();
    }

    static constexpr bool IsEmpty() __noexcept {
      return Size() == 0;
    }

  public:
    T __data[N];
  };

  template <typename T>
  struct Array<T, 0> {
    using Type = T;
    static constexpr usize __size = 0;
  public:
    [[nodiscard, gnu::const]]
    constexpr T* data() const __noexcept {
      return nullptr;
    }

    static constexpr usize Size() __noexcept {
      return 0;
    }

    static constexpr usize SizeInBytes() __noexcept {
      return 0;
    }

    [[nodiscard]]
    PtrRange<T> toPtrRange() const __noexcept {
      return { };
    }

    [[nodiscard, gnu::const]]
    constexpr T* begin() const __noexcept {
      return nullptr;
    }

    [[nodiscard, gnu::const]]
    constexpr T* end() const __noexcept {
      return nullptr;
    }

    static constexpr bool IsEmpty() __noexcept {
      return true;
    }
  };

  template <typename T, typename...TT>
  Array(T&&, TT&&...) -> Array<__decay(T), sizeof...(TT) + 1>;

  Array() -> Array<void, 0>;
} // namespace hc::common
