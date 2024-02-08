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

namespace hc::bootstrap {
  using NtReturn = long;

  template <typename Ret, typename...Args>
  using StdCall = Ret(&)(Args...);

  template <typename...Args>
  using NtCall = StdCall<NtReturn, Args...>;

  enum class Syscall : u32 {
    GetCurrentProcessorNumber = 0,
    Close,
    TestAlert,
    MaxValue = TestAlert
  };

  inline C::EnumArray<B::SyscallValue, Syscall> __syscalls_ {};

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
} // namespace hc::bootstrap
