//===- Common/Strings.hpp -------------------------------------------===//
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
//  Defines common functions used for string operations.
//
//===----------------------------------------------------------------===//

#pragma once

#include "Features.hpp"
#include <Meta/Traits.hpp>

#if __has_feature(cxx_constexpr_string_builtins)
# define __common_strings_cxpr constexpr
#else
# define __common_strings_cxpr
#endif

#define __COMMON_ATTRS __always_inline __common_strings_cxpr

static_assert(__is_reserved(char8_t));
HC_HAS_BUILTIN(char_memchr);
HC_HAS_BUILTIN(memchr);
HC_HAS_BUILTIN(memcmp);
HC_HAS_BUILTIN(strchr);
HC_HAS_BUILTIN(strcmp);
HC_HAS_BUILTIN(strncmp);
HC_HAS_BUILTIN(strlen);

namespace hc::common {
  template <typename T>
  concept __char_type = 
    meta::is_same<T, char>           ||
    meta::is_same<T, char8_t>        ||
    meta::is_same<T, signed char>    ||
    meta::is_same<T, unsigned char>;
  
  template <typename T>
  concept __xchar_type = 
    __char_type<T>             ||
    meta::is_same<T, wchar_t>  ||
    meta::is_same<T, char16_t> ||
    meta::is_same<T, char32_t>;

  //====================================================================//
  // Constexpr Memory Functions
  //====================================================================//

  template <__char_type T>
  __COMMON_ATTRS void*
   __vmemchr(const T* haystack, T needle, usize len) {
    return __builtin_memchr(haystack, int(needle), /* sizeof(T) == 1 */ len);
  }

  __COMMON_ATTRS char* 
   __memchr(const char* haystack, char needle, usize len) {
    return __builtin_char_memchr(haystack, int(needle), len);
  }

  template <__char_type T>
  __COMMON_ATTRS int
   __memcmp(const T* lhs, const T* rhs, usize len) {
    return __builtin_memcmp(lhs, rhs, /* sizeof(T) == 1 */ len);
  }

  //====================================================================//
  // Non-constexpr Memory Functions
  //====================================================================//

  template <typename T>
  requires(!__char_type<T>) __always_inline void*
   __vmemchr(const T* haystack, int needle, usize len) {
    return __builtin_memchr(haystack, needle, __sizeof(T) * len);
  }

  __always_inline wchar_t* 
   __memchr(const wchar_t* haystack, wchar_t needle, usize len) {
    return __builtin_wmemchr(haystack, needle, len);
  }

  template <typename T>
  __always_inline T* 
   __memchr(const T* haystack, int needle, usize len) {
    return (T*)vmemchr(haystack, needle, __sizeof(T) * len);
  }

  __always_inline int
   __memcmp(const wchar_t* lhs, const wchar_t* rhs, usize len) {
    return __builtin_wmemcmp(lhs, rhs, len);
  }

  template <typename T>
  requires(!__char_type<T>) __always_inline int
   __memcmp(const T* lhs, const T* rhs, usize len) {
    return __builtin_memcmp(lhs, rhs, __sizeof(T) * len);
  }

  //====================================================================//
  // String Functions
  //====================================================================//

  __COMMON_ATTRS const char*
   __strchr(const char* haystack, char needle) {
    return __builtin_strchr(haystack, int(needle));
  }

  __COMMON_ATTRS char*
   __strchr(char* haystack, char needle) {
    return __builtin_strchr(haystack, int(needle));
  }

  __COMMON_ATTRS int
   __strcmp(const char* lhs, const char* rhs) {
    return __builtin_strcmp(lhs, rhs);
  }

  __COMMON_ATTRS int
   __strncmp(const char* lhs, const char* rhs, usize len) {
    return __builtin_strncmp(lhs, rhs, len);
  }

  __COMMON_ATTRS usize
   __strlen(const char* S) {
    return __builtin_strlen(S);
  }

  //====================================================================//
  // Wide String Functions
  //====================================================================//

  __COMMON_ATTRS const wchar_t*
   __wstrchr(const wchar_t* haystack, wchar_t needle) {
    return __builtin_wcschr(haystack, needle);
  }

  __COMMON_ATTRS wchar_t*
   __wstrchr(wchar_t* haystack, wchar_t needle) {
    return __builtin_wcschr(haystack, needle);
  }

  __COMMON_ATTRS int
   __wstrcmp(const wchar_t* lhs, const wchar_t* rhs) {
    return __builtin_wcscmp(lhs, rhs);
  }

  __COMMON_ATTRS int
   __wstrncmp(const wchar_t* lhs, const wchar_t* rhs, usize len) {
    return __builtin_wcsncmp(lhs, rhs, len);
  }

  __COMMON_ATTRS usize
   __wstrlen(const wchar_t* S) {
    return __builtin_wcslen(S);
  }

} // namespace hc::common

//======================================================================//
// API
//======================================================================//

namespace hc {
  inline usize stringlen(const char* S) {
    $tail_return common::__strlen(S);
  }

  inline usize stringlen(const wchar_t* S) {
    $tail_return common::__wstrlen(S);
  }

  inline usize stringlen(const char32_t* S) {
    usize len = 0;
    while (S[len++]);
    return len;
  }

  template <common::__xchar_type T>
  requires meta::is_same_size<char, T>
  __always_inline usize stringlen(const T* S) {
    const char* const cstr = 
      reinterpret_cast<const char*>(S);
    return stringlen(cstr);
  }

  template <common::__xchar_type T>
  requires meta::is_same_size<wchar_t, T>
  __always_inline usize stringlen(const T* S) {
    const wchar_t* const wcstr = 
      reinterpret_cast<const wchar_t*>(S);
    return stringlen(wcstr);
  }
} // namespace hc

#undef __COMMON_ATTRS
