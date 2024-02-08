//===- Bootstrap/StubParser.cpp -------------------------------------===//
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

#include <Bootstrap/StubParser.hpp>
#include <Bootstrap/ModuleParser.hpp>
#include <Common/Casting.hpp>
#include <Common/Refl.hpp>
#include <Common/Traits.hpp>

#define $StubErr(ty) $Err(StubError::ty)
#define $MatchInstr(I, e) \
if (primaryOpcode<Instruction::I> == e) \
  return Instruction::I;

using namespace hc::bootstrap;
namespace C = hc::common;
namespace B = hc::bootstrap;

/*
  $Enum((Instruction, u32),
    (MovImmToEax,     0xB8),
    (MovImmToEcx,     0xB9),
    (MovImmToEdx,     0xBA),
    (Retn16,          0xC2),
    (Retn,            0xC3),
    (CallImm,         0xE8),
    // 2 Byte
    (XorEcx,          0x33'C9),
    (MovEspToEdx,     0x8B'D4),
    (CallEdx,         0xFF'D2),
    (Sysenter,        0x0F'34),
    (Syscall,         0x0F'05),
    // 3+ Byte
    (MovRcxToR10,     0x4C'8B'D1),
    (LeaEspOffToEdx,  0x8D'54'24),
    (CallLargePtr,    0x64'FF'15),
    (Unknown)
  );
*/


namespace {
  COFFModule& NtModule() __noexcept {
    static thread_local COFFModule M = 
      ModuleParser::GetParsedModule("ntdll.dll").some();
    return M;
  }

  template <Instruction I>
  __global u8 primaryOpcode = u32(I) & 0xFF;

  u32 extract_syscall(const u8* bytes) {
    // Sanity checking
    __hc_assert(*bytes == primaryOpcode<Instruction::MovImmToEax>);
    return *hc::ptr_cast<const u32>(bytes + 1);
  }

  StubResult do_parsing(const u8* bytes) {
    Instruction I = Instruction::Unknown;
    SyscallValue call = ~u32(0);
    bool did_syscall = false;
    while (I != Instruction::Retn) {
      I = get_instruction(bytes);
      if __expect_false(I == Instruction::Unknown)
        return $StubErr(IllegalInstruction);
      if __expect_false(I == Instruction::Retn16)
        return $StubErr(UnexpectedC2Retn);
      if (I == Instruction::MovImmToEax)
        call = extract_syscall(bytes);
      else if (I == Instruction::Syscall)
        did_syscall = true;
      bytes += instruction_size(I);
    }
    if __expect_false(call == ~u32(0) || !did_syscall)
      return $StubErr(SyscallNotFound);
    return $Ok(call);
  }
} // namespace `anonymous`

Instruction B::get_instruction(const u8* bytes) {
    const u8 primary = *bytes;
    $MatchInstr(Syscall, primary)
    else $MatchInstr(MovRcxToR10, primary)
    else $MatchInstr(Jne, primary)
    else $MatchInstr(Retn, primary)
    else $MatchInstr(MovImmToEax, primary)
    else $MatchInstr(MovImmToEcx, primary)
    else $MatchInstr(MovImmToEdx, primary)
    else $MatchInstr(BitnessCheck, primary)
    else $MatchInstr(Retn16, primary)
    else $MatchInstr(CallImm, primary)
    else $MatchInstr(XorEcx, primary)
    else $MatchInstr(MovEspToEdx, primary)
    else $MatchInstr(CallEdx, primary)
    else $MatchInstr(Sysenter, primary)
    else $MatchInstr(LeaEspOffToEdx, primary)
    else $MatchInstr(CallLargePtr, primary)
    return Instruction::Unknown;
  }

StubResult B::parse_stub(C::StrRef S) {
  if (!S.beginsWith("Nt"))
    return $StubErr(NonNTFunction);
  void* stub = NtModule().resolveExportRaw(S);
  if (stub == nullptr)
    return $StubErr(UnknownFunction);
  return do_parsing(hc::ptr_cast<const u8>(stub));
}
