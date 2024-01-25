//===- Common/Memory.hpp --------------------------------------------===//
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

#include "Features.hpp"
#include "Fundamental.hpp"

#define $launder(expr...) __builtin_launder(expr)
#define $offsetof(name, ty...) __builtin_offsetof(ty, name)

HC_HAS_BUILTIN(is_constant_evaluated);
HC_HAS_BUILTIN(addressof);
HC_HAS_BUILTIN(offsetof);
HC_HAS_BUILTIN(launder);
HC_HAS_BUILTIN(memcpy_inline);
HC_HAS_BUILTIN(memset_inline);

namespace hc::common {
  template <usize N>
  void* __vmemcpy(void* __restrict dst, const void* __restrict src) {
    if constexpr(N == 0) return nullptr;
    if __expect_false(!dst || !src) return nullptr;
    __builtin_memcpy_inline(dst, src, N);
    return dst;
  }

  template <usize N>
  void* __vmemset(void* dst, int ch) {
    if constexpr(N == 0) return nullptr;
    if __expect_false(!dst) return nullptr;
    __builtin_memset_inline(dst, ch, N);
    return dst;
  }

  template <usize N, typename T>
  requires(__is_trivially_copyable(T))
  T* __memcpy(T* __restrict dst, const T* __restrict src) {
    if constexpr(N == 0) return nullptr;
    if __expect_false(!dst || !src) return nullptr;
    __builtin_memcpy_inline(dst, src, __sizeof(T) * N);
    return dst;
  }

  template <usize N, typename T, usize M>
  requires(__is_trivially_copyable(T))
  auto __memcpy(T(&dst)[N], const T(&src)[M]) -> T(&)[N] {
    static constexpr usize Sz = (N >= M) ? N : M;
    __builtin_memcpy_inline(dst, src, __sizeof(T) * Sz);
    return dst;
  }

  template <usize N, typename T>
  requires(__is_trivially_copyable(T))
  T* __memset(T* dst, int ch) {
    if constexpr(N == 0) return nullptr;
    if __expect_false(!dst) return nullptr;
    __builtin_memset_inline(dst, ch, __sizeof(T) * N);
    return dst;
  }

  template <usize N, typename T>
  requires(__is_trivially_copyable(T))
  auto __array_memset(T(&dst)[N], int ch) -> T(&)[N] {
    static constexpr usize len = __sizeof(T) * N;
    __builtin_memset_inline(dst, ch, len);
    return dst;
  }

  template <usize N, typename T>
  __always_inline T* __zero_memory(T* dst) {
    if constexpr(N == 0) return nullptr;
    if __expect_false(!dst) return nullptr;
    return memset<N>(dst, 0);
  }

  template <usize N, typename T>
  __always_inline auto __zero_memory(T(&dst)[N]) -> T(&)[N] {
    return __array_memset(dst, 0);
  }
} // namespace hc::common

namespace hc::common {
  template <typename T>
  [[nodiscard, gnu::nodebug, gnu::always_inline]]
  __visibility(hidden) inline constexpr auto 
   __addressof(T& t) __noexcept {
    return __builtin_addressof(t);
  }

  template <typename T>
  T* __addressof(T&&) = delete;

  // bit_cast

  template <typename To, typename From>
  concept __is_bit_castable =
    (sizeof(To) == sizeof(From))  && 
    (sizeof(From) == sizeof(To))  &&
    __is_trivially_copyable(To)   &&
    __is_trivially_copyable(From);

  template <typename To, typename From>
  [[nodiscard, gnu::always_inline, gnu::nodebug]]
  __visibility(hidden) inline constexpr To
   __bit_cast(const From& from) __noexcept
   requires __is_bit_castable<To, From> {
    if $is_consteval() {
      return __builtin_bit_cast(To, from);
    } else {
      To to;
      __vmemcpy<sizeof(To)>(&to, &from);
      return to;
    }
  }

  // launder

  template <typename T>
  [[nodiscard, gnu::nodebug]] 
  __visibility(hidden) constexpr T*
   __launder(T* src) __noexcept {
    static_assert(!__is_function(T));
    return __builtin_launder(src);
  }

  void* __launder(void*) = delete;
  void* __launder(const void*) = delete;
} // namespace hc::common

namespace hc::common {
  struct Mem {
    static void* VCopy(void* dst, const void* src, usize len);
    static void* VSet(void* dst, int ch, usize len);
  };
} // namespace hc::common
