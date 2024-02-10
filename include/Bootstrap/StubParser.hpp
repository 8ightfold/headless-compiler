//===- Bootstrap/StubParser.hpp -------------------------------------===//
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
//  Tiny specialized assembly parser for extracting syscalls from
//  NT syscall stubs. Do not try this at home...
//  Only x64 and generic retn WOW64 implementations are valid.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Result.hpp>
#include <Common/StrRef.hpp>
#include <Common/TaggedEnum.hpp>

namespace hc::bootstrap {
  $Enum((Instruction, u32),
    (Jne,             0x75),        // jne $+??
    (MovImmToEax,     0xB8),        // mov eax, ??
    (MovImmToEcx,     0xB9),        // mov ecx, ??
    (MovImmToEdx,     0xBA),        // mov edx, ??
    (Retn16,          0xC2),        // retn ??
    (Retn,            0xC3),        // ret
    (CallImm,         0xE8),        // call $+??
    // 2 Byte
    (XorEcx,          0xC9'33),     // xor ecx, ecx
    (MovEspToEdx,     0xD4'8B),     // mov edx, esp
    (CallEdx,         0xD2'FF),     // call edx
    (KEnter,          0x34'0F),     // sysenter
    (KCall,           0x05'0F),     // syscall
    // 3+ Byte
    (MovRcxToR10,     0xD1'8B'4C),  // mov r10, rcx
    (LeaEspOffToEdx,  0x24'54'8D),  // lea edx, [esp+??]
    (BitnessCheck,    0x25'04'F6),  // test BYTE PTR ds:??, 0x1
    (CallLargePtr,    0x15'FF'64),  // call large DWORD PTR fs:??
    (Unknown,         0x9A)
  );

  $StrongEnum((StubError),
    (Success),
    (IllegalInstruction),
    (UnexpectedC2Retn),
    (UnknownFunction),
    (NonNTFunction),
    (SyscallNotFound)
  );

  constexpr usize instruction_size(Instruction I) {
    switch (I) {
     case Jne:            return 2;
     case MovImmToEax:
     case MovImmToEcx:
     case MovImmToEdx:    return 5;
     case Retn16:         return 3;
     case Retn:           return 1;
     case CallImm:        return 5;
     // 2 Byte
     case XorEcx:
     case MovEspToEdx:
     case CallEdx:
     case KEnter:
     case KCall:          return 2;
     // 3+ Byte
     case MovRcxToR10:    return 3;
     case LeaEspOffToEdx: return 4;
     case BitnessCheck:   return 8;
     case CallLargePtr:   return 7;
     default:             return 0;
    }
  }

  using SyscallValue = u32;
  using StubResult = common::Result<SyscallValue, StubError>;

  /// Matches an instruction against the small set of allowed opcodes.
  Instruction get_instruction(const u8* bytes);
  StubResult parse_stub(common::StrRef S);
} // namespace hc::bootstrap
