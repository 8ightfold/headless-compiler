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
#include <Meta/ASM.hpp>

#define HC_FORCE_INTEL _ASM_NOPREFIX

using namespace hc::sys::win;

/*
extern "C" $ASM_func(void,
 _ZN2hc3sys3win11ContextSave7CaptureEPS2_, (ContextSave* ctx),
  "pushfq",
  "movq   %rax, 0x78(%rcx)",

  // Store GP
  "movq   %rcx, 0x80(%rcx)",
  "movq   %rdx, 0x88(%rcx)",
  "movq   %rbx, 0x90(%rcx)",
  "movq   %rbp, 0xa0(%rcx)",

  // Store return address (Rip)
  "movq   8(%rsp), %rax",
  "movq   %rax, 0xf8(%rcx)",

  // Store stack address (Rsp)
  "leaq   16(%rsp), %rax",
  "movq   %rax, 0x98(%rcx)",
  
  // Store GP (cont.)
  "movq   %rsi, 0xa8(%rcx)",
  "movq   %rdi, 0xb0(%rcx)",
  "movq   %r8,  0xb8(%rcx)",
  "movq   %r9,  0xc0(%rcx)",
  "movq   %r10, 0xc8(%rcx)",
  "movq   %r11, 0xd0(%rcx)",
  "movq   %r12, 0xd8(%rcx)",
  "movq   %r13, 0xe0(%rcx)",
  "movq   %r14, 0xe8(%rcx)",
  "movq   %r15, 0xf0(%rcx)",

  // Store flags
  "movq   (%rcx), %rax",
  "mov    %eax, 0x44(%rcx)",

  // Store segments
  "movw   %cs, 0x38(%rcx)",
  "movw   %ds, 0x3a(%rcx)",
  "movw   %es, 0x3c(%rcx)",
  "movw   %fs, 0x3e(%rcx)",
  "movw   %gs, 0x40(%rcx)",
  "movw   %ss, 0x42(%rcx)",

  // Store XMM
  "movaps %xmm0,  0x1a0(%rcx)",
  "movaps %xmm1,  0x1b0(%rcx)",
  "movaps %xmm2,  0x1c0(%rcx)",
  "movaps %xmm3,  0x1d0(%rcx)",
  "movaps %xmm4,  0x1e0(%rcx)",
  "movaps %xmm5,  0x1f0(%rcx)",
  "movaps %xmm6,  0x200(%rcx)",
  "movaps %xmm7,  0x210(%rcx)",
  "movaps %xmm8,  0x220(%rcx)",
  "movaps %xmm9,  0x230(%rcx)",
  "movaps %xmm10, 0x240(%rcx)",
  "movaps %xmm11, 0x250(%rcx)",
  "movaps %xmm12, 0x260(%rcx)",
  "movaps %xmm13, 0x270(%rcx)",
  "movaps %xmm14, 0x280(%rcx)",
  "movaps %xmm15, 0x290(%rcx)",

  // Store legacy
  "fxsave  0x100(%rcx)",
  "stmxcsr 0x34(%rcx)",

  "addq  $0x8, %rsp",
  "ret"
)
*/

extern "C" $ASM_func(void,
 _ZN2hc3sys3win11ContextSave7CaptureEPS2_, (ContextSave* ctx),
  "pushfq",
  "mov qword ptr [rcx + 0x78], rax",

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

  "add rsp, 8",
  "ret"
)
