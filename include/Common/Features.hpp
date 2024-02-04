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

#define HC_MARK_DELETED(name) \
  name(const name&) = delete; \
  name(name&&)      = delete

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

#define __clpragma(...) _Pragma(__hc_stringify(clang __VA_ARGS__))
#define __cldebug(...) __clpragma(__debug __VA_ARGS__)

#define __always_inline __attribute__((always_inline)) inline
#define __visibility(ty) __attribute__((__visibility__(#ty)))
#define __aligned(n) __attribute__((aligned(n)))

#if __clang_major__ >= 17
# define __prefer_type(name) [[clang::preferred_type(name)]]
#else
# define __prefer_type(name)
#endif

#define __expect_false(expr...) (__builtin_expect(bool(expr), 0))
#define __expect_true(expr...)  (__builtin_expect(bool(expr), 1))
#define __unpredictable(expr...)  (__builtin_unpredictable(bool(expr)))
#define __global inline constexpr
#define __noexcept noexcept(!HC_EXCEPTIONS)

template <typename T>
[[gnu::always_inline, gnu::nodebug]]
constexpr __remove_reference_t(T)&& __hc_move(T&& t) __noexcept {
  return static_cast<__remove_reference_t(T)&&>(t);
}

#define __hc_fwd(expr...) static_cast<decltype(expr)&&>(expr)
#define __hc_move(expr...) ::__hc_move(expr)

#if _HC_DEBUG
# define __hc_unreachable(msg...) ::__hc_dbg_unreachable()
#else
# define __hc_unreachable(...) $unreachable
#endif // __hc_unreachable

#if _HC_DEBUG
# define __hc_assert(expr...) \
  [&] () __attribute__((always_inline, artificial)) { \
    if __expect_false(!bool(expr)) \
      __builtin_debugtrap();       \
  }();
#else
# define __hc_assert(...) (void)(0)
#endif // __hc_assert

#if _HC_CHECK_INVARIANTS
# define __hc_invariant(expr...) __hc_assert(expr)
#else
# define __hc_invariant(expr...) (void)(0)
#endif // __hc_invariant

#define $is_consteval() (__builtin_is_constant_evaluated())
#define $launder(expr...) __builtin_launder(expr)
#define $offsetof(name, ty...) __builtin_offsetof(ty, name)

#define $tail_return [[clang::musttail]] return
#define $unreachable __builtin_unreachable()

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

extern "C" { 
#if !__has_builtin(__builtin_stack_address)
  __attribute__((noinline)) static 
   void* __builtin_stack_address(void) {
# if __has_builtin(__builtin_frame_address)
    return __builtin_frame_address(0);
# else
    char byte_aligned = 0;
    // Force optimization barrier
    char* volatile ptr = &byte_aligned;
    return ptr;
# endif
  }
#endif

  __attribute__((cold, noreturn))
  inline void __hc_dbg_unreachable(void) {
    if constexpr(_HC_DEBUG) {
      __builtin_debugtrap();
    }
    __builtin_unreachable();
  }
}
