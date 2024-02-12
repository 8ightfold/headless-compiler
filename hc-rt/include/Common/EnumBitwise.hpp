//===- Common/EnumBitwise.hpp ---------------------------------------===//
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

#include <Common/Features.hpp>

#define $MarkBitwiseEx(T, U)                      \
__ndbg_inline constexpr bool operator!(T L)       \
{ return !(U)(L); }                               \
__ndbg_inline constexpr T operator~(T L)          \
{ return T(~(U)(L)); }                            \
__ndbg_inline constexpr T operator|(T L, T R)     \
{ return T((U)(L) | (U)(R)); }                    \
__ndbg_inline constexpr T operator&(T L, T R)     \
{ return T((U)(L) & (U)(R)); }                    \
__ndbg_inline constexpr T operator^(T L, T R)     \
{ return T((U)(L) ^ (U)(R)); }                    \
__ndbg_inline constexpr T& operator|=(T& L, T R)  \
{ return L = T(L | R); }                          \
__ndbg_inline constexpr T& operator&=(T& L, T R)  \
{ return L = T(L & R); }                          \
__ndbg_inline constexpr T& operator^=(T& L, T R)  \
{ return L = T(L ^ R); }

#define $MarkBitwise(T)                           \
 static_assert(__is_enum(T),                      \
  "$MarkBitwise can only be used with enums.");   \
 $MarkBitwiseEx(T, __underlying_type(T))
