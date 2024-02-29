//===- Sys/Core/Nt/Generic.hpp --------------------------------------===//
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
#include <Common/EnumBitwise.hpp>
#include <Common/Fundamental.hpp>
#include "Handles.hpp"

#define __$NtRange(ex, L, H) (((u32)ex >= (L##ULL)) && ((u32)ex <= (H##ULL)))
#define $NtOk(ex...)   __$NtRange((ex), 0x00000000, 0x3FFFFFFF)
#define $NtInfo(ex...) __$NtRange((ex), 0x40000000, 0x7FFFFFFF)
#define $NtWarn(ex...) __$NtRange((ex), 0x80000000, 0xBFFFFFFF)
#define $NtErr(ex...)  __$NtRange((ex), 0xC0000000, 0xFFFFFFFF)

#define $NtSuccess(ex...) ($NtOk(ex) || $NtInfo(ex))
#define $NtFail(ex...)    ($NtWarn(ex) || $NtErr(ex))

namespace hc::sys::win {
  using bootstrap::StaticUnicodeString;
  using UnicodeString = bootstrap::Win64UnicodeString;
  using WNameRange    = common::PtrRange<wchar_t>;
  using NtStatus      = i32;
  using Long          = i32;
  using ULong         = u32;
  using DWord         = ULong;
  using Boolean       = bool; // AKA. u8
  
  union LargeInt {
    __ndbg_inline operator i64() const {
      return this->quad;
    }
    __ndbg_inline LargeInt& operator=(i64 I) {
      this->quad = I;
      return *this;
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
    __ndbg_inline ULargeInt& operator=(u64 I) {
      this->quad = I;
      return *this;
    }
  public:
    u64 quad = 0UL;
    struct {
      u32 low;
      u32 high;
    };
  };

  //====================================================================//
  // Enumerations
  //====================================================================//

  enum class AccessMask : ULong {
    ReadData          = 0x000001,
    ReadAttributes    = 0x000080,
    ReadEA            = 0x000008,
    ReadControl       = 0x020000,

    WriteData         = 0x000002,
    WriteAttributes   = 0x000100,
    WriteEA           = 0x000010,
    WriteDAC          = 0x040000,
    WriteOwner        = 0x080000,

    Delete            = 0x010000,
    Execute           = 0x000020,
    Sync              = 0x100000,

    StdRightsRequired = 0x0F0000,
    StdRightsRead     = ReadControl,
    StdRightsWrite    = ReadControl,
    StdRightsExec     = ReadControl,

    StdRightsAll      = 0x1F0000,
    SpRightsAll       = 0x00FFFF,
  };

  struct AccessMaskSpecific {
    static constexpr ULong max = ULong(AccessMask::SpRightsAll);
    static constexpr ULong npos = max + 1;
  public:
    __always_inline constexpr operator AccessMask() const {
      __hc_invariant(data < npos);
      return AccessMask(this->data);
    }
    __ndbg_inline explicit constexpr operator ULong() const {
      return this->data;
    }
  public:
    ULong data = 0UL;
  };

  enum class ObjAttribMask : ULong {
    Inherit             = 0x00002,
    Permanent           = 0x00010,
    Exclusive           = 0x00020,
    CaseInsensitive     = 0x00040,
    OpenIf              = 0x00080,
    OpenLink            = 0x00100,
    KernelHandle        = 0x00200,
    ForceAccessCheck    = 0x00400,
    IgnoreImpDeviceMap  = 0x00800,
    DoNotReparse        = 0x01000,
    __ValidAttributes   = 0x01FF2,
  };


  $MarkBitwise(AccessMask)
  $MarkBitwise(ObjAttribMask)
  $MarkBitwiseEx(AccessMaskSpecific, ULong)

  struct ObjectAttributes {
    ULong length  = sizeof(ObjectAttributes);
    FileObjHandle   root_directory = nullptr;
    UnicodeString*  object_name = nullptr;
    ObjAttribMask   attributes = ObjAttribMask::CaseInsensitive;
    void*           security_descriptor = nullptr;
    void*           security_QOS = nullptr;
  };
} // namespace hc::sys::win

namespace hc::sys {
  using NtSyscall = bootstrap::Syscall;
  using win::NtStatus;

  template <NtSyscall C, typename Ret = NtStatus>
  [[gnu::force_align_arg_pointer]]
  __always_inline Ret __stdcall isyscall(auto...args) {
    if constexpr (_HC_TRUE_DEBUG)
      $tail_return bootstrap::__checked_syscall<C, Ret>(args...);
    else
      $tail_return bootstrap::__syscall<C, Ret>(args...);
  }

  inline win::UnicodeString make_unicode_string(const wchar_t* S) {
    if __expect_false(!S) 
      return { };
    return win::UnicodeString::New(const_cast<wchar_t*>(S));
  }
} // namespace hc::sys
