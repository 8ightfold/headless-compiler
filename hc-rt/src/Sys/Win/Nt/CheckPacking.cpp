//===- Sys/Win/Nt/CheckPacking.cpp ----------------------------------===//
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
//  Does checks to see if Nt types are correctly packed.
//
//===----------------------------------------------------------------===//

#include "Info.hpp"

using namespace hc::sys;
using namespace hc::sys::win;

#define PACKING_TEST(ty) static_assert(__fsiwrap_v<FS##ty##Info>, \
  #ty "'s fields are packed incorrectly. Please report this.")

namespace {
  static constexpr usize __fsisz = 16u;

  template <typename FSInfoType>
  using __fsiwrap = FSInfoClassWrapper<FSInfoType, __fsisz>;

  template <typename T>
  static constexpr bool __fsiwrap_v = 
    ($offsetof(__ex_data, __fsiwrap<T>) == sizeof(T));
} // namespace `anonymous`

PACKING_TEST(Attribute);
PACKING_TEST(Control);
PACKING_TEST(Device);
PACKING_TEST(DriverPath);
PACKING_TEST(FullSize);
PACKING_TEST(ObjectID);
PACKING_TEST(SectorSize);
PACKING_TEST(Size);
PACKING_TEST(Volume);

#undef PACKING_TEST

//////////////////////////////////////////////////////////////////////////

#include "Except.hpp"

#define PACKING_TEST(obj, offset, member) \
 static_assert($offsetof(member, obj) == offset, \
  #obj "::" #member " is offset incorrectly. Please report this.")

#define GP_TEST(offset, member) \
 PACKING_TEST(ContextGPReg, offset, member)
#define XMM_TEST(offset, member) \
 PACKING_TEST(ContextXMMReg, offset, member)
#define CTX_TEST(offset, member) \
 PACKING_TEST(ContextSave, offset, member)

GP_TEST(0x0,  rax);
GP_TEST(0x8,  rcx);
GP_TEST(0x10, rdx);
GP_TEST(0x18, rbx);
GP_TEST(0x20, rsp);
GP_TEST(0x28, rbp);
GP_TEST(0x30, rsi);
GP_TEST(0x38, rdi);
GP_TEST(0x40, r8);
GP_TEST(0x48, r9);
GP_TEST(0x50, r10);
GP_TEST(0x58, r11);
GP_TEST(0x60, r12);
GP_TEST(0x68, r13);
GP_TEST(0x70, r14);
GP_TEST(0x78, r15);
GP_TEST(0x80, rip);

XMM_TEST(0x0,   header);
XMM_TEST(0x20,  legacy);
XMM_TEST(0xa0,  xmm0);
XMM_TEST(0x190, xmm15);

static_assert(sizeof(ExceptionRecord) == 0x98);
static_assert(sizeof(ContextSave) == 0x4d0);
CTX_TEST(0x0,   p1h);
CTX_TEST(0x8,   p2h);
CTX_TEST(0x10,  p3h);
CTX_TEST(0x18,  p4h);
CTX_TEST(0x20,  p5h);
CTX_TEST(0x28,  p6h);
CTX_TEST(0x30,  ctx_flags);
CTX_TEST(0x34,  mx_csr);
CTX_TEST(0x38,  seg_cs);
CTX_TEST(0x3a,  seg_ds);
CTX_TEST(0x3c,  seg_es);
CTX_TEST(0x3e,  seg_fs);
CTX_TEST(0x40,  seg_gs);
CTX_TEST(0x42,  seg_ss);
CTX_TEST(0x44,  eflags);
CTX_TEST(0x48,  dr0);
CTX_TEST(0x50,  dr1);
CTX_TEST(0x58,  dr2);
CTX_TEST(0x60,  dr3);
CTX_TEST(0x68,  dr6);
CTX_TEST(0x70,  dr7);
CTX_TEST(0x78,  GP);
CTX_TEST(0x100, XMM);
CTX_TEST(0x300, vec_register);
CTX_TEST(0x4a0, vec_control);
CTX_TEST(0x4a8, dbg_control);
CTX_TEST(0x4b0, lbr_to_rip);
CTX_TEST(0x4b8, lbr_from_rip);
CTX_TEST(0x4c0, lex_to_rip);
CTX_TEST(0x4c8, lex_from_rip);
