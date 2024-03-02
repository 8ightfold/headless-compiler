//===- Phase0/Chkstk.cpp --------------------------------------------===//
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
//  Ensures the stack is committed for the current program. Even though
//  it is unnecessary with our compiler settings, we supply it to avoid
//  linker errors when using opaquely.
//
//===----------------------------------------------------------------===//

#include <Common/Fundamental.hpp>
#include <Meta/ASM.hpp>

// For more info:
// https://nullprogram.com/blog/2024/02/05
// https://github.com/skeeto/w64devkit/blob/master/src/libchkstk.S

extern "C" {
  $ASM_alias(
  void, (__chkstk, ___chkstk_ms), (uptrdiff size));

  [[gnu::used]] $ASM_func(
  void, ___chkstk_ms, (uptrdiff size),
    "push %rax",
    "push %rcx",
    "mov  %gs:(0x10), %rcx",
    "neg  %rax",
    "add  %rsp, %rax",
    "jb   one",
    "xor  %eax, %eax",
   $lbl(zero)
     "sub $0x1000, %rcx",
     "test %eax, (%rcx)",
   $lbl(one)
     "cmp %rax, %rcx",
    "ja   zero",
    "pop  %rcx",
    "pop  %rax",
    "ret"
  )
}
