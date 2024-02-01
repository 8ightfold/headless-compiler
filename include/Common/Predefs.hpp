//===- Common/Predefs.hpp -------------------------------------------===//
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
//  Does basic checks for compiler validity. Only Clang 10+ is supported.
//
//===----------------------------------------------------------------===//

#pragma once

#if defined(__clang__) && (__clang_major__ >= 10)
// All good!
#else
# error Invalid compiler! Only Clang versions >= 10 are supported.
#endif

#if __cplusplus < 202002L
# error C++20 or greater is required.
#endif

#ifndef __has_builtin
# error The macro __has_builtin is required.
#endif

#ifndef __has_feature
# error The macro __has_feature is required.
#endif

#ifdef __is_identifier
# undef  __is_reserved
# define __is_reserved(id) (!__is_identifier(id))
# pragma clang final(__is_reserved)
#else
# error The macro __is_identifier is required.
#endif

//=== General Macros ===//

#define $cat(x, y) __hc_cat(x, y)
#define __hc_cat(x, y) x ## y

#define $stringify(...) __hc_stringify(__VA_ARGS__)
#define __hc_stringify(...) #__VA_ARGS__

//=== Mode Macros ===//

#ifndef _HC_DEBUG
# define _HC_DEBUG 0
#endif

#define _HC_RELEASE (!_HC_DEBUG)

#if _HC_DEBUG && _HC_ENABLE_LTO
# error LTO cannot be used in debug mode!
#endif

//=== Platform Macros ===//

#if defined(_WIN64)
# define HC_PLATFORM_WIN64 1
#elif defined(__linux__)
# define HC_PLATFORM_LINUX 1
#else
# error Unsupported platform!
#endif

#ifndef HC_PLATFORM_WIN64
# define HC_PLATFORM_WIN64 0
#endif

#ifndef HC_PLATFORM_LINUX
# define HC_PLATFORM_LINUX 0
#endif

//=== Architecture Macros ===//

#if defined(__i386__) || defined(__i386)
# define HC_ARCH_X32 1
# define HC_ARCH_PTR_SIZE 4
#elif defined(__amd64__) || defined(__amd64)
# define HC_ARCH_X64 1
# define HC_ARCH_PTR_SIZE 8
#else
# error Unsupported architecture!
#endif

#ifndef HC_ARCH_X32
# define HC_ARCH_X32 0
#endif

#ifndef HC_ARCH_X64
# define HC_ARCH_X64 0
#endif
