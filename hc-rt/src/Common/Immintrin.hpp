//===- Common/Immintrin.hpp -----------------------------------------===//
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
//  To keep my sanity, I'm only implementing small sections for now.
//  May autogenerate more later...
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Fundamental.hpp>
#include "SSEVec.hpp"

#define __SIMD_ATTRS(target) static inline \
 __attribute__((always_inline, nodebug, \
  __target__(#target), __min_vector_width__(128)))

namespace hc::rt {

//======================================================================//
// SSE2
//======================================================================//

#ifdef __SSE2__
__SIMD_ATTRS(sse2) Gmi128 __sse_set_u8(
 u8 a, u8 b, u8 c, u8 d, u8 e, u8 f, u8 g, u8 h,
 u8 i, u8 j, u8 k, u8 l, u8 m, u8 n, u8 o, u8 p) {
  return (Gmi128) Gv128 {
    p, o, n, m, l, k, j, i, 
    h, g, f, e, d, c, b, a
  };
}

__SIMD_ATTRS(sse2) Gmi128 __sse_cmpeq_i8(Gmi128 lhs, Gmi128 rhs) {
  return (Gmi128)(Gvi128(lhs) == Gvi128(rhs));
}
__SIMD_ATTRS(sse2) Gmi128 __sse_cmpeq_u8(Gmi128 lhs, Gmi128 rhs) {
  return (Gmi128)(Gv128(lhs) == Gv128(rhs));
}

__SIMD_ATTRS(sse2) Gmi128 __sse_max_i8(Gmi128 lhs, Gmi128 rhs) {
  return __builtin_elementwise_max(lhs, rhs);
}
__SIMD_ATTRS(sse2) Gmi128 __sse_max_u8(Gmi128 lhs, Gmi128 rhs) {
  return (Gmi128)__builtin_elementwise_max(Gmu128(lhs), Gmu128(rhs));
}

__SIMD_ATTRS(sse2) i32 __sse_movemask_i8(Gmi128 mask) {
  return __builtin_ia32_pmovmskb128(Gvi128(mask));
}

__SIMD_ATTRS(sse2) Gmi128 __sse_xor(Gmi128 lhs, Gmi128 rhs) {
  return (Gmi128)(Gmu128(lhs) ^ Gmu128(rhs));
}
#endif // __SSE2__

//======================================================================//
// SSSE3
//======================================================================//

#ifdef __SSSE3__
__SIMD_ATTRS(ssse3) Gmi128 __sse_shuffle_i8(Gmi128 V, Gmi128 mask) {
  return (Gmi128)__builtin_ia32_pshufb128(Gvi128(V), Gvi128(mask));
}
#endif // __SSSE3__

//======================================================================//
// SSE4.x
//======================================================================//

#if defined(__SSE4_1__) || defined(__SSE4_2__)
__SIMD_ATTRS(sse4.1) i32 __sse_testz(Gmi128 lhs, Gmi128 rhs) {
  return __builtin_ia32_ptestz128(lhs, rhs);
}
#endif // __SSE4_x__

} // namespace hc::rt

#undef __SIMD_ATTRS
