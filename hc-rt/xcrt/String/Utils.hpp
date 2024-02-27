//===- String/Utils.hpp ---------------------------------------------===//
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
//  Adapted from LLVM's libc string_utils.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Casting.hpp>
#include <Common/Fundamental.hpp>
#include <Common/InlineMemcpy.hpp>
#include <Common/InlineMemset.hpp>
// #include <Common/Memory.hpp>
#include <Parcel/BitSet.hpp>

namespace xcrt {
  // For if other platforms are ever supported...
  inline constexpr bool do_unsafe_multibyte_ops = true;

  template <typename Ch>
  inline constexpr usize __make_mask() {
    usize out = 0xFF;
    for (int I = 0; I < sizeof(Ch); ++I)
      out = (out << __bitcount) | 0xFF;
    return out;
  }

  template <typename Int, typename SpacingType = char>
  inline constexpr Int repeat_byte(Int byte) {
    constexpr usize byteMask = __make_mask<SpacingType>();
    constexpr usize byteOff  = __bitsizeof(SpacingType);
    Int res = 0;
    byte = byte & byteMask;
    for (usize I = 0; I < sizeof(Int); ++I)
      res = (res << byteOff) | byte;
    return res;
  }

  template <typename Int, typename Char>
  inline constexpr bool has_zeros(Int block) {
    static_assert(sizeof(Char) <= 2);
    constexpr usize off = (__bitsizeof(Char) - 8);
    constexpr Int loBits = repeat_byte<Int, Char>(0x01);
    constexpr Int hiBits = repeat_byte<Int, Char>(0x80) << off;
    Int subtracted = block - loBits;
    return (subtracted & (~block) & hiBits) != 0;
  }

  // [w]strlen:

  template <typename Int, typename Char>
  inline usize xstringlen_wide_read(const Char* src) {
    constexpr usize alignTo = sizeof(Int);
    const Char* S = src;
    // Align the pointer to Int.
    for (; uptr(S) % alignTo != 0; ++S) {
      if (*S == Char(L'\0'))
        return usize(S - src);
    }
    // Read through blocks.
    for (auto* SI = hc::ptr_cast<const Int>(S);
     !has_zeros<Int, Char>(*SI); ++SI) {
      S = hc::ptr_cast<const Char>(SI);
    }
    // Find the null character.
    for (; *S != Char(L'\0'); ++S);
    return usize(S - src);
  }

  template <typename Char>
  [[maybe_unused]] inline usize
   xstringlen_byte_read(const Char* src) {
    const Char* S = src;
    for (; *S != Char(L'\0'); ++S);
    return usize(S - src);
  }

  template <typename Char>
  inline usize xstringlen(const Char* src) {
    static_assert(sizeof(Char) <= 2);
    if constexpr (do_unsafe_multibyte_ops) {
      using ReadType = hc::common::intn_t<4 * sizeof(Char)>;
      return xstringlen_wide_read<ReadType, Char>(src);
    } else {
      return xstringlen_byte_read<Char>(src);
    }
  }

  __always_inline usize stringlen(const char* src) {
    return xstringlen<char>(src);
  }
  __always_inline usize wstringlen(const wchar_t* src) {
    return xstringlen<wchar_t>(src);
  }

  // [w]strchr:

  template <typename Int, typename Char>
  inline void* xFFC_wide_read(const Char* src, Char C, const usize n) {
    using UChar = hc::common::uintty_t<Char>;
    constexpr usize alignTo = sizeof(Int);
    constexpr usize incOff  = sizeof(Int) / sizeof(Char);

    const UChar* S = hc::ptr_cast<const UChar>(src);
    usize cur = 0;
    // Align the pointer to Int
    for (; uptr(S) % alignTo != 0 && cur < n; ++S, ++cur) {
      if (*S == C)
        return hc::ptr_castex<UChar>(S);
    }
    // Read through blocks.
    const Int C_mask = repeat_byte<Int, Char>(C);
    for (auto* SI = hc::ptr_cast<const Int>(S);
     !has_zeros<Int, Char>((*SI) ^ C_mask) && cur < n;
     ++SI, cur += incOff) {
      S = hc::ptr_cast<const UChar>(SI);
    }
    // Find match in block.
    for (; *S != C && cur < n; ++S, ++cur);

    return (*S != C || cur >= n) 
      ? nullptr : hc::ptr_castex<UChar>(S);
  }

  template <typename Char>
  [[maybe_unused]] inline void*
   xFFC_byte_read(const Char* S, Char C, usize n) {
    using UChar = hc::common::uintty_t<Char>;
    for (; n && *S != C; --n, ++S);
    return n ? hc::ptr_castex<UChar>(S) : nullptr;
  }

  template <typename Char>
  inline void* xfind_first_char(
   const Char* S, Char C, usize max_read) {
    static_assert(sizeof(Char) <= 2);
    if constexpr (do_unsafe_multibyte_ops) {
      using ReadType = hc::common::intn_t<4 * sizeof(Char)>;
      // Check if the overhead of aligning and generating a mask
      // is greater than the overlead of just doing a direct search.
      if (max_read > (alignof(ReadType) * 4))
        return xFFC_wide_read<ReadType, Char>(S, C, max_read);
    }
    return xFFC_byte_read<Char>(S, C, max_read);
  }

  inline char* find_first_char(
   const char* S, char C, usize max_read) {
    void* const P = xfind_first_char<char>(S, C, max_read);
    return static_cast<char*>(P);
  }

  inline wchar_t* wfind_first_char(
   const wchar_t* S, wchar_t C, usize max_read) {
    void* const P = xfind_first_char<wchar_t>(S, C, max_read);
    return static_cast<wchar_t*>(P);
  }

  // misc.

  // Returns the maximum offset that contains characters not found in `seg`.
  inline usize compliment_span(const char* src, const char* seg) {
    const u8* S = hc::ptr_cast<const u8>(src);
    hc::parcel::BitSet<256> bitset;
    for (; *seg; ++seg)
      bitset.set(*hc::ptr_cast<const u8>(seg));
    for (; *S && !bitset.test(*S); ++S);
    return hc::ptr_cast<const char>(S) - src;
  }
} // namespace xcrt
