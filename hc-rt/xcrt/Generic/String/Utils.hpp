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
#include <Common/Limits.hpp>
#include <Common/Pair.hpp>
// #include <Common/Memory.hpp>
#include <Parcel/BitSet.hpp>
#include <Std/__algorithm/max.hpp>

namespace xcrt {
// For if other platforms are ever supported...
__global bool do_unsafe_multibyte_ops = true;
__global usize longNeedleThreshold = 32U;

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

//======================================================================//
// [w]strlen:
//======================================================================//

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
    using ReadType = hc::intn_t<4 * sizeof(Char)>;
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

//======================================================================//
// [w]strnlen:
//======================================================================//

template <typename Int, typename Char>
inline usize xstringnlen_wide_read(const Char* src, usize n) {
  constexpr usize alignTo = sizeof(Int);
  const Char* S = src;
  // Align the pointer to Int.
  for (; uptr(S) % alignTo != 0; ++S) {
    if (*S == Char(L'\0') || !(n--))
      return usize(S - src);
  }
  // Read through blocks.
  for (auto* SI = hc::ptr_cast<const Int>(S);
   !has_zeros<Int, Char>(*SI); ++SI) {
    S = hc::ptr_cast<const Char>(SI);
    if (n < alignTo) break;
    n -= alignTo;
  }
  // Find the null character.
  for (; *S != Char(L'\0'); ++S) {
    if (!(n--)) break;
  }
  return usize(S - src);
}

template <typename Char>
[[maybe_unused]] inline usize
 xstringnlen_byte_read(const Char* src, usize n) {
  const Char* S = src;
  for (; *S != Char(L'\0'); ++S) {
    if (!(n--)) break;
  }
  return usize(S - src);
}

template <typename Char>
inline usize xstringnlen(const Char* src, usize n) {
  static_assert(sizeof(Char) <= 2);
  if constexpr (do_unsafe_multibyte_ops) {
    using ReadType = hc::intn_t<4 * sizeof(Char)>;
    return xstringnlen_wide_read<ReadType, Char>(src, n);
  } else {
    return xstringnlen_byte_read<Char>(src, n);
  }
}

__always_inline usize stringnlen(const char* src, usize n) {
  return xstringnlen<char>(src, n);
}
__always_inline usize wstringnlen(const wchar_t* src, usize n) {
  return xstringnlen<wchar_t>(src, n);
}

//======================================================================//
// [w]strchr:
//======================================================================//

template <typename Int, typename Char>
inline void* xFFC_wide_read(const Char* src, Char C, const usize n) {
  using UChar = hc::uintty_t<Char>;
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
  using UChar = hc::uintty_t<Char>;
  for (; n && *S != C; --n, ++S);
  return n ? hc::ptr_castex<UChar>(S) : nullptr;
}

template <typename Char>
inline void* xfind_first_char(
 const Char* S, Char C, usize max_read) {
  static_assert(sizeof(Char) <= 2);
  if constexpr (do_unsafe_multibyte_ops) {
    using ReadType = hc::intn_t<4 * sizeof(Char)>;
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

//======================================================================//
// [w]str[n]cmp:
//======================================================================//

template <typename Char, typename Cmp>
inline constexpr i32 xstrcmp(
 const Char* lhs, const Char* rhs, Cmp&& cmp) {
  using ConvType = const hc::uintty_t<Char>*;
  for (; *lhs && !cmp(*lhs, *rhs); ++lhs, ++rhs);
  return cmp(*ConvType(lhs), *ConvType(rhs));
}

template <typename Char, typename Cmp>
inline constexpr i32 xstrncmp(
 const Char* lhs, const Char* rhs, usize n, Cmp&& cmp) {
  using ConvType = const hc::uintty_t<Char>*;
  if __expect_false(n == 0)
    return 0;
  
  for (; n > 1; --n, ++lhs, ++rhs) {
    const Char C = *lhs;
    if (!cmp(C, Char(0)) || cmp(C, *rhs))
      break;
  }
  return cmp(*ConvType(lhs), *ConvType(rhs));
}

//======================================================================//
// [w]strstr:
//======================================================================//

template <bool IsReversed, typename Char>
inline hc::Pair<usize, usize> xCR_lex_search(
 const Char* needle, usize needle_len, usize& period) {
  usize max_suffix = hc::Max<usize>;
  usize Nx = 0; // Needle Ix.
  usize Kx = IsReversed; // Period Ix.
  usize ptmp = IsReversed;

  auto cmp = [](Char a, Char b) -> bool {
    if constexpr (IsReversed)
      return (b < a);
    else
      return (a < b);
  };

  while (Nx + Kx < needle_len) {
    Char a = needle[Nx + Kx];
    // Wraps around on first index.
    Char b = needle[max_suffix + Kx];
    if (cmp(a, b)) {
      Nx += Kx;
      Kx = 1;
      ptmp = Nx - max_suffix;
    } else if (a == b) {
      if (Kx != ptmp) {
        ++Kx;
      } else {
        Nx += ptmp;
        Kx = 1;
      }
    } else {
      max_suffix = Nx++;
      Kx = ptmp = 1;
    }
  }

  if (!IsReversed)
    period = ptmp;
  return {max_suffix, ptmp};
}

template <typename Char>
inline usize critical_factorization(
 const Char* needle, usize needle_len, usize& period) {
  auto [max_suffix, _]
    // Normal
    = xCR_lex_search<false>(needle, needle_len, period);
  auto [max_suffix_r, p]
    // Reversed
    = xCR_lex_search<true>(needle, needle_len, period);
  if (max_suffix_r < max_suffix)
    return max_suffix + 1;
  period = p;
  return max_suffix_r + 1;
}

template <typename Char>
__always_inline bool xFFS_available(
 const Char* S, usize& S_len, usize Sx, usize needle_len) {
  if (Sx + needle_len <= S_len)
    return true;
  S_len += xstringnlen(S + S_len, 512);
  return Sx + needle_len <= S_len;
}

//////////////////////////////////////////////////////////////////////////

/// Thanks glibc!!
template <bool CheckEOL = true, typename Char, typename Cmp>
inline void* xFFS_short_needle(
 const Char* S, usize S_len,
 const Char* needle, usize needle_len,
 Cmp&& cmp
) {
  usize Nx = 0; // Needle index.
  usize Sx = 0; // Haystack index.
  usize period = 0;

  usize suffix = critical_factorization(
    needle, needle_len, period
  );

  /* Perform the search.  Each iteration compares the right half
     first.  */
  if (xstrncmp<Char>(needle, needle + period, suffix, cmp) == 0) {
    /* Entire needle is periodic; a mismatch can only advance by the
	    period, so use memory to avoid rescanning known occurrences
	    of the period.  */
    usize memory = 0;
    Sx = 0;
    while (xFFS_available<Char>(S, S_len, Sx, needle_len)) {
	    /* Scan for matches in right half.  */
	    Nx = std::max(suffix, memory);
	    const Char* pneedle = &needle[Nx];
	    const Char* phaystack = &S[Nx + Sx];
	    while (Nx < needle_len && ((*pneedle++) == (*phaystack++))) {
        ++Nx;
      }

	    if (needle_len <= Nx) {
	      /* Scan for matches in left half.  */
	      Nx = suffix - 1;
	      pneedle = &needle[Nx];
	      phaystack = &S[Nx + Sx];
	      while (memory < Nx + 1 && ((*pneedle--) == (*phaystack--))) {
          --Nx;
        }
	      if (Nx + 1 < memory + 1)
	  	    return hc::ptr_castex<>(S + Sx);
	      /* No match, so remember how many repetitions of period
	  	    on the right half were scanned.  */
	      Sx += period;
	      memory = needle_len - period;
	    } else {
	      Sx += Nx - suffix + 1;
	      memory = 0;
	    }
	  }
  } else {
    const Char* phaystack = nullptr;
    /* The comparison always starts from needle[suffix], so cache it
	    and use an optimized first-character loop.  */
    Char needle_suffix = needle[suffix];

    /* The two halves of needle are distinct; no extra memory is
	    required, and any mismatch results in a maximal shift.  */
    period = std::max(suffix, needle_len - suffix) + 1;
    Sx = 0;
    while (xFFS_available(S, S_len, Sx, needle_len)) {
      Char haystack_char;
      phaystack = &S[suffix + Sx];

      if (*phaystack++ != needle_suffix) {
        phaystack = (Char*) xfind_first_char<Char>(
          phaystack, needle_suffix,
    		  S_len - needle_len - Sx
        );
        if (phaystack == nullptr)
    	    return nullptr;
        Sx = phaystack - &S[suffix];
        phaystack++;
      }
      /* Scan for matches in right half.  */
      Nx = suffix + 1;
      const Char* pneedle = &needle[Nx];
      while (Nx < needle_len) {
        haystack_char = *phaystack++;
        if ((*pneedle++) != haystack_char) {
          if (!haystack_char)
            return nullptr;
    	    break;
    	  }
        ++Nx;
      }
      if constexpr (CheckEOL) {
        /* Update minimal length of S.  */
        if (phaystack > S + S_len)
          S_len = phaystack - S;
      }
      if (needle_len <= Nx) {
        /* Scan for matches in left half.  */
        Nx = suffix - 1;
        pneedle = &needle[Nx];
        phaystack = &S[Nx + Sx];
        while (Nx != hc::Max<usize>) {
          haystack_char = *phaystack++;
    	    if ((*pneedle--) != haystack_char) {
            if (!haystack_char)
              return nullptr;
    	      break;
    	    }
    	    --Nx;
    	  }
        if (Nx == hc::Max<usize>)
    	    return hc::ptr_castex<>(S + Sx);
        Sx += period;
      } else {
        Sx += Nx - suffix + 1;
      }
    }
  }

  return nullptr;
}

template <bool CheckEOL = true, typename Char, typename Cmp>
[[maybe_unused]] inline void* xFFS_long_needle(
 const Char* S, usize S_len,
 const Char* needle, usize needle_len,
 Cmp&& cmp
) {
  // TODO: Long needle
  // https://github.com/lattera/glibc/blob/master/string/str-two-way.h
  return xFFS_short_needle<CheckEOL, Char>(
    S, S_len,
    needle, needle_len,
    __hc_fwd(cmp)
  );
}

template <typename Char, typename Cmp>
inline void* xfind_first_str(
 const Char* S,
 const Char* needle, 
 usize max_read,
 Cmp&& cmp
) {
  static_assert(sizeof(Char) <= 2);
  if (!needle[0])
    return hc::ptr_castex<>(S);

  // Skip until we find the first matching Char.
  S = (const Char*) xfind_first_char(S, needle[0], max_read);
  if (!S || !needle[1])
    return hc::ptr_castex<>(S);

  usize needle_len = xstringlen(needle);
  max_read = xstringnlen(S, needle_len + (256 / sizeof(Char)));
  if (max_read < needle_len)
    return nullptr;

  // Check if we already have a match.
  if (xstrncmp(S, needle, needle_len, cmp) == 0)
    return hc::ptr_castex<>(S);

  if (needle_len < longNeedleThreshold) {
    return xFFS_short_needle<true>(
      S, max_read,
      needle, needle_len,
      __hc_fwd(cmp)
    );
  }

  return xFFS_long_needle<true>(
    S, max_read,
    needle, needle_len,
    __hc_fwd(cmp)
  );
}

inline const char* find_first_str(
 const char* S, const char* needle, usize max_read) {
  const auto cmp = [](char L, char R) -> i32 { return L - R; };
  void* const P = xfind_first_str<char>(S, needle, max_read, cmp);
  return static_cast<const char*>(P);
}

inline const wchar_t* wfind_first_str(
 const wchar_t* S, const wchar_t* needle, usize max_read) {
  const auto cmp = [](wchar_t L, wchar_t R) -> i32 { return L - R; };
  void* const P = xfind_first_str<wchar_t>(S, needle, max_read, cmp);
  return static_cast<const wchar_t*>(P);
}

__always_inline char* find_first_str(
 char* S, const char* needle, usize max_read) {
  const char* const CS = S;
  return (char*) find_first_str(CS, needle, max_read);
}

__always_inline wchar_t* wfind_first_str(
 wchar_t* S, const wchar_t* needle, usize max_read) {
  const wchar_t* const CS = S;
  return (wchar_t*) wfind_first_str(CS, needle, max_read);
}

//======================================================================//
// Misc.
//======================================================================//

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
