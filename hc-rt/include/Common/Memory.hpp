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

HC_HAS_BUILTIN(is_constant_evaluated);
HC_HAS_BUILTIN(addressof);
HC_HAS_BUILTIN(offsetof);
HC_HAS_BUILTIN(launder);
HC_HAS_BUILTIN(memcpy_inline);
HC_HAS_BUILTIN(memset_inline);

namespace hc::common {
  template <usize N>
  void* __vmemcpy(void* __restrict dst, const void* __restrict src) {
    if constexpr (N == 0) return nullptr;
    __builtin_memcpy_inline(dst, src, N);
    return dst;
  }

  template <usize N>
  void* __vmemset(void* dst, int ch) {
    if constexpr (N == 0) return nullptr;
    __builtin_memset_inline(dst, ch, N);
    return dst;
  }

  template <usize N, typename T>
  requires(__is_trivially_copyable(T))
  T* __memcpy(T* __restrict dst, const T* __restrict src) {
    if constexpr (N == 0) return nullptr;
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
    if constexpr (N == 0) return nullptr;
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
  __always_inline T* __zero_memory(T& dst) {
    if constexpr (N == 0) return nullptr;
    return __memset<N>(__builtin_addressof(dst), 0);
  }

  template <usize N, typename T>
  __always_inline auto __zero_memory(T(&dst)[N]) -> T(&)[N] {
    return __array_memset(dst, 0);
  }

  template <typename T>
  requires(__is_trivially_copyable(T))
  T& __clone(T& dst, const T& src) {
    __builtin_memcpy_inline(&dst, &src, __sizeof(T));
    return dst;
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
    static_assert(__is_object(T));
    return __builtin_launder(src);
  }

  void* __launder(void*) = delete;
  void* __launder(const void*) = delete;

  // assume_aligned

  template <usize Align, typename T>
  [[nodiscard, gnu::always_inline]]
  __visibility(hidden) inline constexpr T*
   __assume_aligned(T* p) __noexcept {
    static_assert((Align & (Align - 1)) == 0,
      "Align must be a power of 2.");
    if $is_consteval() {
      return p;
    } else {
      __hc_invariant((uptr)p % Align == 0);
      return static_cast<T*>(
        __builtin_assume_aligned(p, Align));
    }
  }
} // namespace hc::common

namespace hc::common {
  struct Mem {
    static void* VCopy(void* __restrict dst, const void* __restrict src, usize len);
    // TODO: This! lol!
    static void* VSet(void* __restrict dst, int ch, usize len);

    template <typename T>
    requires(__is_trivially_copyable(T))
    __always_inline static T* 
     Copy(T* __restrict dst, const T* __restrict src, usize len) {
      static_assert(!__is_void(T), "Directly use VCopy with void*!");
      (void)Mem::VCopy(dst, src, len * __sizeof(T));
      return dst;
    }

    template <typename T>
    requires(!__is_trivially_copyable(T))
    static T* Copy(T* __restrict dst, const T* __restrict src, usize len) {
      static_assert(!__is_void(T), "Directly use VCopy with void*!");
      for (usize I = 0; I < len; ++I)
        dst[I] = src[I];
      return dst;
    }

    template <typename T>
    requires(__is_trivial(T))
    __always_inline static T* 
     Move(T* __restrict dst, T* __restrict src, usize len) {
      return Mem::Copy(dst, src, len);
    }

    template <typename T>
    requires(!__is_trivial(T))
    static T* Move(T* __restrict dst, T* __restrict src, usize len) {
      for (usize I = 0; I < len; ++I)
        dst[I] = static_cast<T&&>(src[I]);
      return dst;
    }

    template <typename T>
    requires(__is_trivial(T))
    __always_inline static T* 
     Reset(T* __restrict dst, u8 pattern, usize len) {
      return Mem::VSet(dst, int(pattern), len);
    }

    template <typename T>
    requires(!__is_trivial(T))
    static T* Reset(T* __restrict dst, u8, usize len) {
      for (usize I = 0; I < len; ++I)
        dst[I] = T{};
      return dst;
    }

    template <typename T>
    requires(__is_trivially_copyable(T))
    __always_inline static T& Clone(T& dst, const T& src) {
      return __clone(dst, src);
    }

    template <typename T>
    requires(!__is_trivially_copyable(T))
    static T& Clone(T& dst, const T& src) {
      return dst = src;
    }
  };
} // namespace hc::common
