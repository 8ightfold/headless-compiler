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

#define __common_is_same(t, u) (__is_same(t, u) && __is_same(u, t))
#define __common_attrs __always_inline __common_strings_cxpr

static_assert(__is_reserved(char8_t));
HC_HAS_REQUIRED(builtin, __is_same);
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
    meta::is_same<T, signed char>    ||
    meta::is_same<T, unsigned char>  ||
    meta::is_same<T, char8_t>;

  //=== Constexpr Memory Functions ===//

  template <typename T>
  requires(__char_type<T>) __common_attrs void*
   __vmemchr(const T* haystack, T needle, usize len) {
    return __builtin_memchr(haystack, int(needle), /* sizeof(T) == 1 */ len);
  }

  __common_attrs char* 
   __memchr(const char* haystack, char needle, usize len) {
    return __builtin_char_memchr(haystack, int(needle), len);
  }

  template <typename T>
  requires(__char_type<T>) __common_attrs int
   __memcmp(const T* lhs, const T* rhs, usize len) {
    return __builtin_memcmp(lhs, rhs, /* sizeof(T) == 1 */ len);
  }

  //=== Non-constexpr Memory Functions ===//

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

  //=== String Functions ===//

  __common_attrs const char*
   __strchr(const char* haystack, char needle) {
    return __builtin_strchr(haystack, int(needle));
  }

  __common_attrs char*
   __strchr(char* haystack, char needle) {
    return __builtin_strchr(haystack, int(needle));
  }

  __common_attrs int
   __strcmp(const char* lhs, const char* rhs) {
    return __builtin_strcmp(lhs, rhs);
  }

  __common_attrs int
   __strncmp(const char* lhs, const char* rhs, usize len) {
    return __builtin_strncmp(lhs, rhs, len);
  }

  __common_attrs usize
   __strlen(const char* str) {
    return __builtin_strlen(str);
  }

  //=== Wide String Functions ===//

  __common_attrs const wchar_t*
   __wstrchr(const wchar_t* haystack, wchar_t needle) {
    return __builtin_wcschr(haystack, needle);
  }

  __common_attrs wchar_t*
   __wstrchr(wchar_t* haystack, wchar_t needle) {
    return __builtin_wcschr(haystack, needle);
  }

  __common_attrs int
   __wstrcmp(const wchar_t* lhs, const wchar_t* rhs) {
    return __builtin_wcscmp(lhs, rhs);
  }

  __common_attrs int
   __wstrncmp(const wchar_t* lhs, const wchar_t* rhs, usize len) {
    return __builtin_wcsncmp(lhs, rhs, len);
  }

  __common_attrs usize
   __wstrlen(const wchar_t* str) {
    return __builtin_wcslen(str);
  }

} // namespace hc::common

#undef __common_is_same
#undef __common_attrs
