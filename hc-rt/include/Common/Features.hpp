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

#define __clpragma(args...) _Pragma(__hc_stringify(clang args))
#define __cldiag(args...) __clpragma(diagnostic args)
#define __cldebug(...) __clpragma(__debug __VA_ARGS__)

#define __always_inline __attribute__((always_inline, artificial)) inline
#define __ndbg_inline   __attribute__((always_inline, nodebug))    inline
#define __visibility(ty) __attribute__((__visibility__(#ty)))
#define __aligned(n) __attribute__((aligned(n)))

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

#if __has_attribute(counted_by)
# define __counted_by(name) __attribute__((counted_by(name)))
#else
# define __counted_by(name)
#endif

#if __has_attribute(__nodebug__)
# define __nodebug __attribute__((__nodebug__))
#else
# define __nodebug
#endif

#define __expect_false(expr...) (__builtin_expect(bool(expr), 0))
#define __expect_true(expr...)  (__builtin_expect(bool(expr), 1))
#define __unpredictable(expr...)  (__builtin_unpredictable(bool(expr)))
#define __global inline constexpr

#if HC_EXCEPTIONS
# define __throw(ty...) throw(ty)
# define __noexcept noexcept(false)
#else
# define __throw(...)
# define __noexcept noexcept
#endif

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

#if _HC_TRUE_DEBUG
# define __hc_trap() __builtin_debugtrap()
#else
# define __hc_trap() __builtin_trap()
#endif

#if _HC_DEBUG
# define __hc_assert(expr...) \
  [&] () __attribute__((always_inline, artificial)) { \
    if __expect_false(!bool(expr)) \
      __hc_trap();                 \
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
#define $unreachable_msg(message) __builtin_unreachable()

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

extern "C" { 
#if !__has_builtin(__builtin_stack_address)
  __attribute__((noinline)) static 
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
    if constexpr(_HC_DEBUG) {
      __hc_trap();
    }
    __builtin_unreachable();
  }

  __attribute__((cold, noreturn, format(__printf__, 1, 2)))
  int __hc_raw_panic(const char fmt[], ...);
}
