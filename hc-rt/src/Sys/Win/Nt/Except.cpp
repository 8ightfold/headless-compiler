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

#include "Except.hpp"
#include <Common/Casting.hpp>
#include <Common/DefaultFuncPtr.hpp>
#include <Bootstrap/_NtModule.hpp>
#include <Bootstrap/KUserSharedData.hpp>
#include <Bootstrap/Win64KernelDefs.hpp>
#include <Meta/ASM.hpp>
#include <Meta/Unwrap.hpp>

#define HC_FORCE_INTEL _ASM_NOPREFIX

using namespace hc;
using namespace hc::sys::win;
using bootstrap::__NtModule;
using bootstrap::KUSER_SHARED_DATA;
using bootstrap::Win64TEB;

namespace hc::sys {
  using LookupType = RuntimeFunction*(u64, u64*, void*);
  using UnwindType = void*(
    u32, u64, u64, 
    RuntimeFunction*, ContextSave*,
    void**, u64*, void*
  );
  
  static constinit DefaultFuncPtr<LookupType> RtlLookupFunctionEntry {};
  static constinit DefaultFuncPtr<UnwindType> RtlVirtualUnwind {};

  template <typename F>
  static bool load_nt_symbol(
   DefaultFuncPtr<F>& func, StrRef symbol) {
    auto exp = __NtModule()->resolveExport<F>(symbol);
    return func.setSafe($unwrap(exp));
  }

  bool init_SEH_exceptions() {
# define $load_symbol(name) \
   succeeded &= load_nt_symbol(name, #name)
    bool succeeded = true;
    $load_symbol(RtlLookupFunctionEntry);
    $load_symbol(RtlVirtualUnwind);
    return succeeded;
# undef $load_symbol
  }
} // namespace hc::sys

bool ExceptionRecord::CheckDbgStatus() {
  bool status = Win64TEB::LoadPEBFromGS()->being_debugged;
  if (!status) {
    const u8 dbgv = KUSER_SHARED_DATA.KdDebuggerEnabled & 0b11;
    status = (dbgv == 0b11);
  }
  return status;
}

void ExceptionRecord::Raise(ExceptionRecord& record) {
  const bool has_exceptions = init_SEH_exceptions();
  if __expect_false(!has_exceptions)
    return;
  if (!CheckDbgStatus())
    return;
  
  ContextSave ctx;
  ContextSave::Capture(&ctx);

  const u64 pc = ctx.GP.rip;
  u64 image_base = 0;
  RuntimeFunction* entry =
    RtlLookupFunctionEntry(pc, &image_base, nullptr);
  if (!entry) {
    // RtlRaiseStatus
    return;
  }

  void* handler_data = nullptr;
  u64 establisher_frame = 0;
  RtlVirtualUnwind(0, image_base, pc, entry,
    &ctx, &handler_data, &establisher_frame, nullptr);
  record.address = ptr_cast<>(ctx.GP.rip);
  // record.address = __builtin_return_address(0);

  raise_exception(record, &ctx, true);
  return;

  /*
  volatile bool already_printed = false;
  if (!already_printed) {
    already_printed = true;
    raise_exception(record, &ctx, true);
  }
  */
}

/// Like legacy RtlCaptureContext.
$ASM_func(void, ContextSave::Capture, (ContextSave* ctx),
  "pushfq",
  "test rcx, rcx",
  "je   Quit",
  "mov qword ptr [rcx + 0x78], rax",

  // CONTEXT_CONTROL: 0b00001
  // CONTEXT_INTEGER: 0b00010
  // CONTEXT_SEGMENT: 0b00100
  // CONTEXT_FLOAT:   0b01000
  // CONTEXT_TAG:     0x100000
  "mov dword ptr [rcx + 0x30], 0x10000f",

  // Store GP
  "mov qword ptr [rcx + 0x80], rcx",
  "mov qword ptr [rcx + 0x88], rdx",
  "mov qword ptr [rcx + 0x90], rbx",
  "mov qword ptr [rcx + 0xa0], rbp",

  // Store return address (Rip)
  "mov rax, qword ptr [rsp + 8]",
  "mov qword ptr [rcx + 0xf8], rax",

  // Store stack address (Rsp)
  "lea rax, [rsp + 16]",
  "mov qword ptr [rcx + 0x98], rax",

  // Store GP (cont.)
  "mov qword ptr [rcx + 0xa8], rsi",
  "mov qword ptr [rcx + 0xb0], rdi",
  "mov qword ptr [rcx + 0xb8], r8",
  "mov qword ptr [rcx + 0xc0], r9",
  "mov qword ptr [rcx + 0xc8], r10",
  "mov qword ptr [rcx + 0xd0], r11",
  "mov qword ptr [rcx + 0xd8], r12",
  "mov qword ptr [rcx + 0xe0], r13",
  "mov qword ptr [rcx + 0xe8], r14",
  "mov qword ptr [rcx + 0xf0], r15",

  // Store flags
  "mov rax, qword ptr [rcx]",
  "mov dword ptr [rcx + 0x44], eax",

  // Store segments
  "mov  word ptr [rcx + 0x38], cs",
  "mov  word ptr [rcx + 0x3a], ds",
  "mov  word ptr [rcx + 0x3c], es",
  "mov  word ptr [rcx + 0x3e], fs",
  "mov  word ptr [rcx + 0x40], gs",
  "mov  word ptr [rcx + 0x42], ss",

  // Store XMM
  "movaps xmmword ptr [rcx + 0x1a0], xmm0",
  "movaps xmmword ptr [rcx + 0x1b0], xmm1",
  "movaps xmmword ptr [rcx + 0x1c0], xmm2",
  "movaps xmmword ptr [rcx + 0x1d0], xmm3",
  "movaps xmmword ptr [rcx + 0x1e0], xmm4",
  "movaps xmmword ptr [rcx + 0x1f0], xmm5",
  "movaps xmmword ptr [rcx + 0x200], xmm6",
  "movaps xmmword ptr [rcx + 0x210], xmm7",
  "movaps xmmword ptr [rcx + 0x220], xmm8",
  "movaps xmmword ptr [rcx + 0x230], xmm9",
  "movaps xmmword ptr [rcx + 0x240], xmm10",
  "movaps xmmword ptr [rcx + 0x250], xmm11",
  "movaps xmmword ptr [rcx + 0x260], xmm12",
  "movaps xmmword ptr [rcx + 0x270], xmm13",
  "movaps xmmword ptr [rcx + 0x280], xmm14",
  "movaps xmmword ptr [rcx + 0x290], xmm15",

  // Store legacy
  "fxsave  [rcx + 0x100]",
  "stmxcsr dword ptr [rcx + 0x34]",

 "Quit:"
  "add rsp, 8",
  "ret"
)
