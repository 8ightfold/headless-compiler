//===- Sys/Win/Nt/Except.hpp ----------------------------------------===//
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

#include <Common/PadStruct.hpp>
#include "Generic.hpp"

namespace hc::sys::win {
  struct Reg128 {
    u64 low;
    i64 high;
  };

  struct ContextGPReg {
    u64 rax, rcx, rdx, rbx, rsp, rbp;
    u64 rsi, rdi;
    u64 r8, r9, r10, r11, r12, r13, r14, r15;
    u64 rip;
  };

  struct _ContextXMMReg {
    Reg128 header[2];
    Reg128 legacy[8];
    Reg128 xmm0,  xmm1,  xmm2,  xmm3;
    Reg128 xmm4,  xmm5,  xmm6,  xmm7;
    Reg128 xmm8,  xmm9,  xmm10, xmm11;
    Reg128 xmm12, xmm13, xmm14, xmm15;
  };

  using ContextXMMReg = com::PadStruct<_ContextXMMReg, 0x200>;
  static_assert(sizeof(ContextXMMReg) == 0x200);

  ////////////////////////////////////////////////////////////////////////

  struct ExceptionRecord {
    static bool CheckDbgStatus();
    static void Raise(ExceptionRecord& record);

    static ExceptionRecord New(
     i32 code, __nonnull auto* address, auto...args) {
      constexpr usize count = sizeof...(args);
      static_assert(count < 16, "Too many arguments!");
      static_assert((... && (sizeof(args) <= sizeof(uptr))));
      return ExceptionRecord {
        .code = code,
        .address = (void*)(address),
        .nparams = count,
        .info = { reinterpret_cast<uptr>(args)... }
      };
    }

    ExceptionRecord& setArgs(auto...args) {
      constexpr usize count = sizeof...(args);
      static_assert(count < 16, "Too many arguments!");
      static_assert((... && (sizeof(args) <= sizeof(uptr))));

      if constexpr (count > 0) {
        const uptr arg_array[count] {
          reinterpret_cast<uptr>(args)...
        };
        for (usize Ix = 0; Ix < count; ++Ix)
          this->info[Ix] = arg_array[Ix];
      }

      this->nparams = count;
      return *this;
    }

  public:
    i32               code;
    u32               flags = 0;
    ExceptionRecord*  next = nullptr;
    void*             address;
    u32               nparams;
    u64               info[15];
  }; 

  struct alignas(16) ContextSave {
    [[gnu::used, gnu::noinline, gnu::naked]]
    static void Capture(ContextSave* ctx);
  public:
    u64           p1h, p2h, p3h, p4h, p5h, p6h;
    u32           ctx_flags;
    u32           mx_csr;
    u16           seg_cs, seg_ds, seg_es, seg_fs, seg_gs, seg_ss;
    u32           eflags;
    u64           dr0, dr1, dr2, dr3, dr6, dr7;
    ContextGPReg  GP;
    ContextXMMReg XMM;
    Reg128        vec_register[26];
    u64           vec_control;
    u64           dbg_control;
    u64           lbr_to_rip;   // Last branch to RIP
    u64           lbr_from_rip; // Last branch from RIP
    u64           lex_to_rip;   // Last exception to RIP
    u64           lex_from_rip; // Last exception from RIP
  };

  struct RuntimeFunction {
    u32 begin_addr;
    u32 end_addr;
    u32 unwind_info_addr;
  };
} // namespace hc::sys::win
