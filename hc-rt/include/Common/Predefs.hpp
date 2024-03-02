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

#ifndef __cplusplus
# error C++ is required.
#elif __cplusplus < 202002L
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

// Probably the minimum for C++23
#if __cplusplus >= 202100L
# define _HC_CXX23 1
#else
# define _HC_CXX23 0
#endif

//======================================================================//
// General Macros
//======================================================================//

#define __hc_4cat(w, x, y, z) w ## x ## y ## z
#define $4cat(x, y, z) __hc_4cat(w, x, y, z)

#define __hc_3cat(x, y, z) x ## y ## z
#define $3cat(x, y, z) __hc_3cat(x, y, z)

#define __hc_2cat(x, y) x ## y
#define $cat(x, y) __hc_2cat(x, y)
#define $2cat(x, y) __hc_2cat(x, y)

#define __hc_stringify(...) #__VA_ARGS__
#define $stringify(...) __hc_stringify(__VA_ARGS__)

#define $unique(ty...) ::__hc_tyident_<ty> $2cat(__unique_, __COUNTER__)
#define $var(name) $3cat(__v_, name, __COUNTER__)

template <typename T>
using __hc_tyident_ = T;

//======================================================================//
// Mode Macros
//======================================================================//

#ifndef _HC_DEBUG
# define _HC_DEBUG 0
#endif

#ifndef _HC_TRUE_DEBUG
# define _HC_TRUE_DEBUG 0
#endif

#define _HC_RELEASE (!_HC_DEBUG)

#if _HC_DEBUG && _HC_ENABLE_LTO
# error LTO cannot be used in debug mode!
#endif

//======================================================================//
// Platform Macros
//======================================================================//

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

#undef __cdecl
#undef __fastcall
#undef __stdcall
#undef __thiscall

#if HC_PLATFORM_WIN64
# define __eol "\r\n"
# define __cdecl    __attribute__((__cdedl__))
# define __fastcall __attribute__((__fastcall__))
# define __stdcall  __attribute__((__stdcall__))
# define __thiscall __attribute__((__thiscall__))
#else
# define __eol "\n"
# define __cdecl
# define __fastcall
# define __stdcall
# define __thiscall
#endif

//======================================================================//
// Architecture Macros
//======================================================================//

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

//======================================================================//
// Runtime Macros
//======================================================================//

#ifndef _HC_MULTITHREADED
# define _HC_MULTITHREADED 0
#endif

#if !_HC_MULTITHREADED && (RT_MAX_THREADS != 0)
# undef  RT_MAX_THREADS
# define RT_MAX_THREADS 0
#endif
