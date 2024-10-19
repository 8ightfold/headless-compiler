//===- Common/Features.hpp ------------------------------------------===//
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

#include "Predefs.hpp"

#define HC_HAS_REQUIRED(ty, name) static_assert(__has_##ty(name))
#define HC_HAS_BUILTIN(name) static_assert(__has_builtin(__builtin_##name))

// Not currently implemented in C++.
#define _HC_COUNTED_BY_CPP 0

#define HC_MARK_DELETED(name)             \
  name(const name&)            = delete;  \
  name& operator=(const name&) = delete;  \
  name(name&&)                 = delete;  \
  name& operator=(name&&) = delete

#ifndef __HC_INTERNAL__
# ifndef _HC_COMMON_INLINE
#  define _HC_COMMON_INLINE 0
# endif
#else
# undef _HC_COMMON_INLINE
# define _HC_COMMON_INLINE 1
#endif

#if __has_feature(cxx_exceptions)
# define HC_EXCEPTIONS 1
#else
# define HC_EXCEPTIONS 0
#endif // If -fno-exceptions

#if __has_feature(cxx_rtti)
# define HC_RTTI 1
#else
# define HC_RTTI 0
#endif // If -fno-rtti

#if __has_feature(address_sanitizer)
# define HC_ASAN 1
#else
# define HC_ASAN 0
#endif // If -fsanitize=address

#if __has_feature(thread_sanitizer)
# define HC_TSAN 1
#else
# define HC_TSAN 0
#endif // If -fsanitize=thread

#if __has_feature(memory_sanitizer)
# define HC_MEMSAN 1
#else
# define HC_MEMSAN 0
#endif // If -fsanitize=memory

#if __has_feature(dataflow_sanitizer)
# define HC_DFSAN 1
#else
# define HC_DFSAN 0
#endif // If DataFlowSanitizer enabled

#if __has_feature(safe_stack)
# define HC_SAFESTACK 1
#else
# define HC_SAFESTACK 0
#endif // If -fsanitize=safe-stack

#define __clpragma(args...) _Pragma(__hc_stringify(clang args))
#define __cldiag(args...) __clpragma(diagnostic args)
#define __cldebug(...) __clpragma(__debug __VA_ARGS__)
#define __nounroll _Pragma("nounroll")

#define __always_inline __attribute__((always_inline, artificial)) inline
#define __ndbg_inline   __attribute__((always_inline, nodebug))    inline
#define __aligned(n) __attribute__((aligned(n)))
#define __section(sect_str) __attribute__((section(sect_str), used))

#define __visibility(ty) __attribute__((__visibility__(#ty)))
#define __hidden __visibility(hidden)

#if __has_attribute(preferred_type)
# define __prefer_type(name) __attribute__((preferred_type(name)))
#else
# define __prefer_type(name)
#endif

#if __has_attribute(__preferred_name__)
# define __prefer_name(name) __attribute__((__preferred_name__(name)))
#else
# define __prefer_name(name)
#endif

#if _HC_COUNTED_BY_CPP && __has_attribute(counted_by)
# define __counted_by(name) __attribute__((counted_by(name)))
#else
# define __counted_by(name)
#endif

#if __has_attribute(nonnull)
# define __hc_nonnull __attribute__((nonnull))
#else
# define __hc_nonnull
#endif

#if !__is_reserved(__nonnull)
# define __nonnull __hc_nonnull
#endif

#if __has_attribute(__nodebug__)
# define __nodebug __attribute__((__nodebug__))
#else
# define __nodebug
#endif

#if !__has_cpp_attribute(clang::trivial_abi)
# error [[clang::trivial_abi]] is required.
#endif

#if __has_cpp_attribute(clang::lifetimebound)
# define __lifetimebound [[clang::lifetimebound]]
#elif __has_cpp_attribute(_Clang::__lifetimebound__)
# define __lifetimebound [[_Clang::__lifetimebound__]]
#else
# define __lifetimebound
#endif

#if __has_attribute(exclude_from_explicit_instantiation)
# define __exclude_from_explicit_instantiation \
  __attribute__((__exclude_from_explicit_instantiation__))
#else
# define __exclude_from_explicit_instantiation \
  __attribute__((__always_inline__)) inline
#endif

#if _HC_CHECK_INVARIANTS && _HC_DEBUG
# define _HC_HARDENING_SIG d
#elif _HC_DEBUG
# define _HC_HARDENING_SIG e
#else
# define _HC_HARDENING_SIG n
#endif

#if _HC_EXCEPTIONS
# define _HC_EXCEPTION_SIG e
#else
# define _HC_EXCEPTION_SIG n
#endif

#if _HC_MULTITHREADED
# define _HC_THREADING_SIG m
#else
# define _HC_THREADING_SIG n
#endif

#define _HC_ODR_SIG $3cat( \
  _HC_HARDENING_SIG, _HC_EXCEPTION_SIG, _HC_THREADING_SIG)

#define __abi __attribute__((__abi_tag__($stringify(_HC_ODR_SIG))))
#define __abi_hidden __hidden __exclude_from_explicit_instantiation __abi

#if __has_builtin(__builtin_expect_with_probability)
# define __expect_false(expr...) (__builtin_expect_with_probability(bool(expr), 0, 1.0))
# define __expect_true(expr...)  (__builtin_expect_with_probability(bool(expr), 1, 1.0))
# define __likely_false(expr...) (__builtin_expect_with_probability(bool(expr), 0, 0.7))
# define __likely_true(expr...)  (__builtin_expect_with_probability(bool(expr), 1, 0.7))
#else
# define __expect_false(expr...) (__builtin_expect(bool(expr), 0))
# define __expect_true(expr...)  (__builtin_expect(bool(expr), 1))
# define __likely_false(expr...) (bool(expr))
# define __likely_true(expr...)  (bool(expr))
#endif

#if __has_builtin(__builtin_unpredictable)
# define __unpredictable(expr...)  (__builtin_unpredictable(bool(expr)))
#else
# define __unpredictable(expr...)  (bool(expr))
#endif

#define __global inline constexpr
#define __intrnl static constexpr
#define __gmut   inline constinit
#define __imut   static constinit

#if _HC_CXX23
// statics in constexpr functions
# define __cxstatic static
#else
// static not available in constexpr.
# define __cxstatic
#endif

#if HC_EXCEPTIONS
# define __throw(ty...) throw(ty)
# define __noexcept noexcept(false)
#else
// TODO: Make this something...
# define __throw(...)
# define __noexcept noexcept
#endif

template <typename T>
[[nodiscard, gnu::always_inline, gnu::nodebug]] 
inline constexpr __remove_reference_t(T)&&
 __hc_move_(__lifetimebound T&& V) __noexcept {
  return static_cast<__remove_reference_t(T)&&>(V);
}

template <typename T>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
inline constexpr T&& 
 __hc_fwd_(__lifetimebound __remove_reference_t(T)& V) {
  return static_cast<T&&>(V);
}

template <typename T>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
inline constexpr T&& 
 __hc_fwd_(__lifetimebound __remove_reference_t(T)&& V) {
  return static_cast<T&&>(V);
}

#define __hc_fwd(expr...) static_cast<decltype(expr)&&>(expr)
#define __hc_move(expr...) ::__hc_move_(expr)

#if _HC_DEBUG
# define __hc_unreachable(msg...) ::__hc_dbg_unreachable()
#else
# define __hc_unreachable(...) $unreachable
#endif // __hc_unreachable

#if _HC_TRUE_DEBUG
# define __hc_trap() __builtin_debugtrap()
#else
# define __hc_trap() __builtin_trap()
#endif

/// A constexpr-only assertion.
#define __hc_cxassert(expr...) do { \
  if (__builtin_is_constant_evaluated() && !bool(expr)) \
    __builtin_unreachable(); \
} while(0)

#if _HC_DEBUG
/// Checked condition in debug.
# define __hc_assert(expr...) \
  [&] () __attribute__((always_inline, nodebug)) { \
    if (__builtin_expect(!bool(expr), 0)) \
      ::__hc_dbg_unreachable(); \
  }();
/// Use this when you need something to be evaluated in release.
# define __hc_assertOrIdent(expr...) __hc_assert(expr)
#else // !_HC_DEBUG
/// Noop in release.
# define __hc_assert(expr...) __hc_cxassert(expr)
/// Evaluated but unchecked.
# define __hc_assertOrIdent(expr...) (void)(expr)
#endif // _HC_DEBUG

#if _HC_CHECK_INVARIANTS
# define __hc_invariant(expr...) __hc_assert(expr)
#else
# define __hc_invariant(expr...) __hc_cxassert(expr)
#endif // __hc_invariant

#define __hc_clear_padding(expr...) __builtin_clear_padding(&(expr))

#define __hc_todo(name, ret...) \
 do { __hc_unreachable(name " is unimplemented."); \
  return ret; } while(0)

// TODO: Add __hc_trace("msg", args...)

#define $is_consteval() (__builtin_is_constant_evaluated())
#define $launder(expr...) __builtin_launder(expr)
#define $offsetof(name, ty...) __builtin_offsetof(ty, name)

#define $fwd(expr...) ::__hc_fwd_<decltype(expr)>(expr)
#define $mv(expr...) ::__hc_move_(expr)

/// Tail calls another function. Must have a matching signature.
#define $tail_return [[clang::musttail]] return
#define $unreachable __builtin_unreachable()
#define $unreachable_msg(message) __builtin_unreachable()
/// Allows `break` to be used in arbitrary scopes.
#define $scope switch (0) case 0:

namespace hc {

template <typename T>
consteval bool compile_failure() {
  return false;
}

template <auto V>
consteval bool compile_failure() {
  return false;
}

} // namespace hc

#define $compile_failure(ty, ...) static_assert( \
  hc::compile_failure<ty>() __VA_OPT__(, "In "#ty": ") __VA_ARGS__);
#define $flag(n...) (1ULL << (n))

#pragma clang final(__always_inline)
#pragma clang final(__global)
#pragma clang final(__noexcept)

HC_HAS_REQUIRED(attribute, always_inline);
HC_HAS_REQUIRED(attribute, __visibility__);
HC_HAS_REQUIRED(cpp_attribute, clang::musttail);
HC_HAS_BUILTIN(trap);
HC_HAS_BUILTIN(expect);
HC_HAS_BUILTIN(launder);
HC_HAS_BUILTIN(unreachable);
HC_HAS_BUILTIN(is_constant_evaluated);

#define __builtin_return_address(n) \
  __builtin_extract_return_addr(__builtin_return_address(n))

extern "C" { 
#if !__has_builtin(__builtin_stack_address)
  [[maybe_unused, gnu::noinline]] static 
   void* __builtin_stack_address(void) {
# if __has_builtin(__builtin_frame_address)
    return __builtin_frame_address(0);
# else
    char byte_aligned = 1;
    // Force optimization barrier
    char* volatile ptr = &byte_aligned;
    return ptr;
# endif
  }
#endif

  __attribute__((cold, noreturn))
  inline void __hc_dbg_unreachable(void) {
    if constexpr (_HC_DEBUG) __hc_trap();
    __builtin_unreachable();
  }

  /// Invokes the current panic handler and aborts.
  /// TODO: Implement raw_panic
  __attribute__((cold, noreturn, format(__printf__, 1, 2)))
  int __hc_raw_panic(const char fmt[], ...);

  /// Writes a trace to the current handler.
  /// Used for logging what functions were called,
  /// and with which arguments.
  /// TODO: Implement log_trace
  __attribute__((format(__printf__, 1, 2)))
  int __hc_log_trace(const char fmt[], ...);
} // extern "C"

namespace hc {
  namespace bootstrap {}
  namespace boot = bootstrap;
  namespace common {}
  namespace com = common;
  namespace parcel {}
  namespace pcl = parcel;
#if _HC_COMMON_INLINE
  using namespace common;
#endif // _HC_COMMON_INLINE
} // nanespace hc
