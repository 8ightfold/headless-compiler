//===- Common/Fundamental.hpp ---------------------------------------===//
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

HC_HAS_REQUIRED(builtin, __make_signed);
HC_HAS_REQUIRED(builtin, __make_unsigned);

__global decltype(sizeof(0)) __bitcount = __CHAR_BIT__;

//=== Integrals ===//

using ibyte = signed char;
using ubyte = unsigned char;

using i8  = __INT8_TYPE__;
using i16 = __INT16_TYPE__;
using i32 = __INT32_TYPE__;
using i64 = __INT64_TYPE__;

using u8  = __UINT8_TYPE__;
using u16 = __UINT16_TYPE__;
using u32 = __UINT32_TYPE__;
using u64 = __UINT64_TYPE__;

#if __is_reserved(__int128) || defined(__SIZEOF_INT128__)
using i128 = __int128;
using u128 = unsigned __int128;
#elif defined(__hc_i128)
using i128 = __hc_i128;
using u128 = unsigned __hc_i128;
#else
# error No 128-bit int type found!
#endif

using usize = decltype(sizeof(0));
using isize = __make_signed(usize);

using iptrdiff = __make_signed(decltype((u8*)nullptr - (u8*)nullptr));
using uptrdiff = __make_unsigned(iptrdiff);

namespace hc::common {
  template <usize N> struct _IntN;
  template <usize N> struct _UIntN;

  template <> struct  _IntN<1UL>  { using type = i8; };
  template <> struct  _IntN<2UL>  { using type = i16; };
  template <> struct  _IntN<4UL>  { using type = i32; };
  template <> struct  _IntN<8UL>  { using type = i64; };
  template <> struct  _IntN<16UL> { using type = i128; };

  template <> struct _UIntN<1UL>  { using type = u8; };
  template <> struct _UIntN<2UL>  { using type = u16; };
  template <> struct _UIntN<4UL>  { using type = u32; };
  template <> struct _UIntN<8UL>  { using type = u64; };
  template <> struct _UIntN<16UL> { using type = u128; };

  template <usize N> using intn_t  = typename _IntN<N>::type;
  template <usize N> using uintn_t = typename _UIntN<N>::type;
  template <typename T> using intty_t  = intn_t<sizeof(T)>;
  template <typename T> using uintty_t = uintn_t<sizeof(T)>;
} // namespace hc::common

using iptr = hc::common::intty_t<void*>;
using uptr = hc::common::uintty_t<void*>;

using ihalfptr = hc::common::intn_t<sizeof(void*) / 2>;
using uhalfptr = __make_unsigned(ihalfptr);

//=== Floating Point ===//

#if __is_reserved(_Float16)
using f16 = _Float16;
#elif defined(__hc_f16)
using f16 = __hc_f16;
#else
# error No 16-bit float type found!
#endif

using f32 = float;
using f64 = double;

namespace hc::common {
  template <usize N> struct _FloatN;

  template <> struct _FloatN<2UL> { using type = f16; };
  template <> struct _FloatN<4UL> { using type = f32; };
  template <> struct _FloatN<8UL> { using type = f64; };

  template <usize N> 
  using floatn_t = typename _FloatN<N>::type;
} // namespace hc::common

using nullptr_t = decltype(nullptr);

//=== Pseudofunctions ===//

#undef __sizeof
#undef __bitsizeof

namespace hc { 
  struct __dummy { };
  // Proxy void type.
  struct __void  { };
} // namespace hc

template <typename T = void> 
__global usize __sizeof = sizeof(T);

template <> __global usize __sizeof<void> = 0;
template <> __global usize __sizeof<hc::__void> = 0;

template <typename T = void>
__global usize __bitsizeof = __sizeof<T> * __bitcount;

#define __sizeof(...) ::__sizeof<__VA_ARGS__>
#define __bitsizeof(...) ::__bitsizeof<__VA_ARGS__>
