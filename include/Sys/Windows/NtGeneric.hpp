//===- Sys/Windows/NtGeneric.hpp ------------------------------------===//
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

#include <Bootstrap/UnicodeString.hpp>
#include <Bootstrap/Syscalls.hpp>
#include <Common/Fundamental.hpp>

#define __$NtRange(ex, L, H) (((u64)ex >= (L##ULL)) && ((u64)ex <= (H##ULL)))
#define $NtOk(ex...)   __$NtRange((ex), 0x00000000, 0x3FFFFFFF)
#define $NtInfo(ex...) __$NtRange((ex), 0x40000000, 0x7FFFFFFF)
#define $NtWarn(ex...) __$NtRange((ex), 0x80000000, 0xBFFFFFFF)
#define $NtErr(ex...)  __$NtRange((ex), 0xC0000000, 0xFFFFFFFF)

#define $NtSuccess(ex...) ($NtOk(ex) || $NtInfo(ex))
#define $NtFail(ex...)    ($NtWarn(ex) || $NtErr(ex))

namespace hc::sys::win {
  using UnicodeString = bootstrap::Win64UnicodeString;
  using NtStatus  = i32;
  using ULong     = u32;
  using DWord     = ULong;
  using Boolean   = bool; // AKA. u8
  
  union LargeInt {
    __ndbg_inline operator i64() const {
      return this->quad;
    }
  public:
    i64 quad = 0L;
    struct {
      u32 low;
      i32 high;
    };
  };

  union ULargeInt {
    __ndbg_inline operator u64() const {
      return this->quad;
    }
  public:
    u64 quad = 0UL;
    struct {
      u32 low;
      u32 high;
    };
  };
} // namespace hc::sys::win

namespace hc::sys {
  using NtSyscall = bootstrap::Syscall;
  using win::NtStatus;

  template <NtSyscall C, typename Ret = NtStatus>
  inline Ret __stdcall isyscall(auto...args) {
    if constexpr (_HC_TRUE_DEBUG)
      $tail_return bootstrap::__checked_syscall<C, Ret>(args...);
    else
      $tail_return bootstrap::__syscall<C, Ret>(args...);
  }
} // namespace hc::sys
