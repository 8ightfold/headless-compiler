//===- Common/MemUtils.hpp ------------------------------------------===//
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

#include <Common/Fundamental.hpp>
#include <Common/Memory.hpp>
#include "Immintrin.hpp"

#define __SIMD_ATTRS(target) static inline \
 __attribute__((__always_inline__, __nodebug__, \
  __target__(#target), __min_vector_width__(128)))

namespace hc::rt {
  template <typename T>
  concept __is_memelem_v = 
    __is_scalar(T) ||
    __is_vector<T> ||
    __is_Array<T>;
  
  template <meta::is_integral T>
  __always_inline constexpr T bswap(T V) {
    __cxstatic constexpr usize sz = sizeof(V);
    using U = __make_unsigned(T);
    if constexpr (sz == 8) {
      return (T) __builtin_bswap64(U(V));
    } else if constexpr (sz == 4) {
      return (T) __builtin_bswap32(U(V));
    } else if constexpr (sz == 2) {
      return (T) __builtin_bswap16(U(V));
    } else if constexpr (sz == 1) {
      return V;
    }
  }

  template <typename T>
  inline constexpr T load(const u8* src) {
    static_assert(__is_memelem_v<T>);
    if constexpr (__is_scalar(T) || __is_vector<T>) {
      T V;
      common::__vmemcpy<sizeof(T)>(&V, src);
      return V;
    } else {
      using U = __array_type_t<T>;
      T out;
      for (usize I = 0; I < __array_size_v<T>; ++I)
        out[I] = load<U>(src + (I + sizeof(U)));
      return out;
    }
  }

  template <typename T>
  inline constexpr T load(const u8* src, usize off) {
    return load<T>(src + off);
  }

  template <meta::is_integral T>
  inline constexpr T load_be(const u8* src) {
    return bswap(load<T>(src));
  }

  template <meta::is_integral T>
  inline constexpr T load_be(const u8* src, usize off) {
    return bswap(load<T>(src + off));
  }

  template <typename T>
  inline constexpr void store(u8* dst, T V) {
    static_assert(__is_memelem_v<T>);
    if constexpr (__is_scalar(T) || __is_vector<T>) {
      common::__vmemcpy<sizeof(T)>(dst, &V);
    } else {
      using U = __array_type_t<T>;
      for (usize I = 0; I < __array_size_v<T>; ++I)
        store<U>(dst + (I * sizeof(U)), V[I]);
    }
  }

  template <typename T>
  inline constexpr T splat(u8 V) {
    static_assert(__is_memelem_v<T>);
    if constexpr (__is_scalar(T)) {
      return T(~0) / T(0xFF) * T(V);
    } else if constexpr(__is_vector<T>) {
      T out;
      for (usize I = 0; I < sizeof(T); ++I)
        out[I] = V;
      return out;
    } else {
      using U = __array_type_t<T>;
      const auto u = splat<U>(V);
      T out;
      for (usize I = 0; I < __array_size_v<T>; ++I)
        out[I] = u;
      return out;
    }
  }

  //====================================================================//
  // Comparison
  //====================================================================//

  template <typename T>
  concept __cmp_is_expensive = (__sizeof(T) > 4);

  template <typename T>
  inline bool eq(const u8* lhs, const u8* rhs, usize off) {
    return load<T>(lhs, off) == load<T>(rhs, off);
  }

  template <typename T>
  inline u32 neq(const u8* lhs, const u8* rhs, usize off) {
    return load<T>(lhs, off) ^ load<T>(lhs, off);
  }

  template <typename T>
  inline i32 cmp_eq(const u8* lhs, const u8* rhs, usize off) {
    return i32(load_be<T>(lhs, off))
         - i32(load_be<T>(rhs, off));
  }

  template <typename T>
  inline i32 cmp_neq(const u8* lhs, const u8* rhs, usize off);

  ////////////////////////////////////////////////////////////////////////
  // Specializations for >= u64
  inline constexpr i32 cmp_eq_u32(u32 lhs, u32 rhs) {
    const i64 diff = i64(lhs) - i64(rhs);
    return i32((diff >> 1) | (diff & 0xFFFF));
  }

  inline constexpr i32 cmp_neq_u64(u64 lhs, u64 rhs) {
    constexpr i64 POS = +5;
    constexpr i64 NEG = -5;
    return (lhs < rhs) ? NEG : POS;
  }

  template <>
  inline i32 cmp_eq<u32>(const u8* lhs, const u8* rhs, usize off) {
    const auto x = load_be<u32>(lhs, off);
    const auto y = load_be<u32>(rhs, off);
    return cmp_eq_u32(x, y);
  }

  template <>
  inline u32 neq<u64>(const u8* lhs, const u8* rhs, usize off) {
    return !eq<u64>(lhs, rhs, off);
  }

  template <>
  inline i32 cmp_neq<u64>(const u8* lhs, const u8* rhs, usize off) {
    const auto x = load_be<u64>(lhs, off);
    const auto y = load_be<u64>(rhs, off);
    return cmp_neq_u64(x, y);
  }

  ////////////////////////////////////////////////////////////////////////
  // Specializations for mi128
  #if defined(__SSE4_1__)
  inline Gmi128 __bytewise_reverse(Gmi128 V) {
    return __sse_shuffle_i8(V, __sse_set_u8(
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
    ));
  }
  inline u16 be_cmp_mask(Gmi128 max, Gmi128 V) {
    return u16(__sse_movemask_i8(
        __bytewise_reverse(__sse_cmpeq_i8(max, V)))
    );
  }

  template <>
  inline bool eq<Gmi128>(const u8* lhs, const u8* rhs, usize off) {
    const auto x = load<Gmi128>(lhs, off);
    const auto y = load<Gmi128>(rhs, off);
    const auto xored = __sse_xor(x, y);
    return __sse_testz(xored, xored) == 1;
  }

  template <>
  inline u32 neq<Gmi128>(const u8* lhs, const u8* rhs, usize off) {
    const auto x = load<Gmi128>(lhs, off);
    const auto y = load<Gmi128>(rhs, off);
    const auto xored = __sse_xor(x, y);
    return __sse_testz(xored, xored) == 0;
  }

  template <>
  inline i32 cmp_neq<Gmi128>(const u8* lhs, const u8* rhs, usize off) {
    const auto x = load<Gmi128>(lhs, off);
    const auto y = load<Gmi128>(rhs, off);
    const auto max = __sse_max_u8(x, y);
    const u16 le   = be_cmp_mask(max, y);
    const u16 ge   = be_cmp_mask(max, x);
    return i32(ge) - i32(le);
  }

  #endif // __SSE4_1__

} // namespace hc::rt

#undef __SIMD_ATTRS
