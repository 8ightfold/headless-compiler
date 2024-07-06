//===- Bootstrap/Syscalls.hpp ---------------------------------------===//
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
//  Windows syscall API. Go crazy.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Fundamental.hpp>
#include <Common/EnumArray.hpp>
#include <Common/TaggedEnum.hpp>

// For more info:
// http://undocumented.ntinternals.net
// https://www.geoffchappell.com/studies/windows/win32/ntdll/api/native.htm

namespace hc::bootstrap {
  using NtReturn = long;
  using ULong = unsigned long;

  template <typename Ret, typename...Args>
  using StdCall = Ret(&)(Args...);

  template <typename...Args>
  using NtCall = StdCall<NtReturn, Args...>;

  enum class Syscall : u32 {
#  define $NtGen(name) name,
#  include "Syscalls.mac"
    MaxValue
  };
  $MarkName(Syscall);

  constexpr const char* __refl_fieldname(Syscall E) {
    switch (E) {
#    define $NtGen(name) \
      case Syscall::name: return $stringify(name);
#    include "Syscalls.mac"
      default: return nullptr;
    }
  }

  inline constexpr auto& __refl_fieldarray(Syscall) { \
    static constexpr Syscall A[] {
#    define $NtGen(name) Syscall::name,
#    include "Syscalls.mac"
      Syscall::MaxValue
    };
    return A;
  }

  static constexpr u32 __invalid_syscall_ = ~0UL;
  extern constinit common::EnumArray<u32, Syscall> __syscalls_;

  /// Ensure `__syscalls_` has been initialized, otherwise you'll
  /// just have random nigh-undebuggable errors.
  template <Syscall C, typename Ret = NtReturn, typename...Args>
  [[gnu::noinline, gnu::naked]]
  inline Ret __stdcall __syscall(Args...args) {
    __asm__ volatile ("movq %%rcx, %%r10;\n"::);
    __asm__ volatile (
      "mov %[val], %%eax;\n"
      "syscall;\n"
      "retn;\n"
      :: [val] "r"(__syscalls_.__data[u32(C)])
    );
  }

  template <Syscall C, typename Ret = NtReturn, typename...Args>
  __always_inline Ret __stdcall __checked_syscall(Args...args) {
    __hc_assert(__syscalls_[C] != __invalid_syscall_);
    $tail_return __syscall<C, Ret>(args...);
  }

 #ifndef __XCRT__
  static struct _SyscallLoader {
    _SyscallLoader();
  } __sys_loader_ {};
 #endif // __XCRT__?

  extern void force_syscall_reload();
  extern bool are_syscalls_loaded();
} // namespace hc::bootstrap
