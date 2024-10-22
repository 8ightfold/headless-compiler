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
#include <Sys/Win/Except.hpp>

#include <Common/Casting.hpp>
#include <Common/DefaultFuncPtr.hpp>
#include <Bootstrap/_NtModule.hpp>
#include <Bootstrap/KUserSharedData.hpp>
#include <Bootstrap/Win64KernelDefs.hpp>
#include <Meta/ASM.hpp>
#include <Meta/Unwrap.hpp>

// For more info:
// https://windows-internals.com/cet-on-windows
// https://richard-ac.github.io/posts/SEH
// https://github.com/mic101/windows/blob/master/WRK-v1.2/base/ntos/dbgk/dbgkport.c#L30

#define HC_FORCE_INTEL _ASM_NOPREFIX
#define $load_symbol(name) \
  succeeded &= load_nt_symbol(name, #name)

using namespace hc;
using namespace hc::sys::win;
using bootstrap::__NtModule;
using bootstrap::KUSER_SHARED_DATA;
using bootstrap::KUSER_XState;
using bootstrap::Win64TEB;

namespace {
  struct ContextChunk {
    i32 offset;
    u32 length;
  };

  struct ContextSaveEx {
    ContextChunk all;
    ContextChunk legacy;
    ContextChunk xstate;
  };
} // namespace `anonymous`

namespace hc::sys {
  using LookupType = RuntimeFunction*(u64 pc, u64* image_base, void*);
  using UnwindType = void*(u32 type, u64 image_base, u64 pc,
    RuntimeFunction* entry, ContextSave* ctx, void** data,
    u64* establisher_frame, void*
  );

  using ExCtxLen2Type = u32(u32 flags, u32* len, u64 compaction_mask);
  using ExCtxInit2Type = u32(void* ctx, u32 flags,
    ContextSaveEx** ctxex, u64 compaction_mask
  );
  
  using CaptureCtx2Type  = void(ContextSave* ctx);
  using LegacyCtxLocType = ContextSave*(ContextSaveEx* ctxex, u32* len);
  
  __imut DefaultFuncPtr<LookupType>       RtlLookupFunctionEntry {};
  __imut DefaultFuncPtr<UnwindType>       RtlVirtualUnwind {};
  __imut DefaultFuncPtr<ExCtxLen2Type>    RtlGetExtendedContextLength2 {};
  __imut DefaultFuncPtr<ExCtxInit2Type>   RtlInitializeExtendedContext2 {};
  __imut DefaultFuncPtr<CaptureCtx2Type>  RtlCaptureContext2 {};
  __imut DefaultFuncPtr<LegacyCtxLocType> RtlLocateLegacyContext {};

  template <typename F>
  static bool load_nt_symbol(
   DefaultFuncPtr<F>& func, StrRef symbol) {
    if __expect_true(func.isSet())
      return true;
    auto exp = __NtModule()->resolveExport<F>(symbol);
    return func.setSafe($unwrap(exp));
  }

  bool init_SEH_exceptions() {
    static bool has_succeeded = false;
    if __expect_false(!has_succeeded) {
      bool succeeded = true;
      $load_symbol(RtlLookupFunctionEntry);
      $load_symbol(RtlVirtualUnwind);
      // TODO: Split up as version check
      $load_symbol(RtlGetExtendedContextLength2);
      $load_symbol(RtlInitializeExtendedContext2);
      $load_symbol(RtlCaptureContext2);
      $load_symbol(RtlLocateLegacyContext);
      has_succeeded = succeeded;
    }
    return has_succeeded;
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

#if 1
void ExceptionRecord::Raise(ExceptionRecord& record) {
  const bool has_exceptions = init_SEH_exceptions();
  if __expect_false(!has_exceptions)
    return;
  if (!CheckDbgStatus())
    return;

  u32 ctx_flags = 0x10000b;
  u64 compaction_mask = 0;

  $scope {
    // Theres some bitfield flags here in the release,
    // not sure what they do exactly.
    if (KUSER_XState.EnabledFeatures == 0 /* && !some_bitfield.@42 */)
      break;
    ctx_flags |= 0x40;
    if ((KUSER_XState.ControlFlags & 2) == 0)
      break;
    const u64 enabled_features =
      KUSER_XState.EnabledFeatures |
      KUSER_XState.EnabledSupervisorFeatures;
    u64 feature_mask = enabled_features | 0x8000000000000000;
    // Hope and pray our bit is always 0. From my testing the entire
    // value is usually cleared, but there's no way to know for sure.
    if (enabled_features & 0x800 /* && !some_bitfield.@42 */) {
      feature_mask = (enabled_features & 0xfffffffffffff7ff);
      feature_mask |= 0x8000000000000000;
    }
    // Ngl ion know what any of these fuckin values mean
    // TODO: MAKE SOME GOD DAMN TESTS!!!
    compaction_mask = (feature_mask & 0xfffffffffff9ffff);
  }

  u32 ctxex_len = 0;
  RtlGetExtendedContextLength2(
    ctx_flags, &ctxex_len, compaction_mask);
  void* raw_ctx = __builtin_alloca(ctxex_len);

  ContextSaveEx* ctxex = nullptr;
  if (RtlInitializeExtendedContext2(
   raw_ctx, ctx_flags, &ctxex, compaction_mask))
    return;
  
  u32 ctx_len = 0;
  ContextSave* ctx =
    RtlLocateLegacyContext(ctxex, &ctx_len);
  if __expect_false(!ctx || ctx_len == 0) return;
  RtlCaptureContext2(ctx);

  const u64 pc = ctx->GP.rip;
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
    ctx, &handler_data, &establisher_frame, nullptr);
  record.address = ptr_cast<>(ctx->GP.rip);

  raise_exception(record, ctx, true);
  return;
}
#else
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

  raise_exception(record, &ctx, true);
  return;
}
#endif

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
