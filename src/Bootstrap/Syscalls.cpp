//===- Bootstrap/Syscalls.cpp ---------------------------------------===//
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

#include <Bootstrap/Syscalls.hpp>
#include <Bootstrap/StubParser.hpp>
#include <Common/Unwrap.hpp>

#undef $AssignSyscall
#define $AssignSyscall(name) __syscalls_[Syscall::name] = \
  B::parse_stub($stringify(Nt##name)).valueOr(~0UL)

using namespace hc::bootstrap;
namespace C = hc::common;
namespace B = hc::bootstrap;

namespace {
  i32 load_syscalls() {
#  define $NtGen(name)  $AssignSyscall(name);
#  include <Bootstrap/Syscalls.mac>
    return 1;
  }
} // namespace `anonymous`

void B::force_syscall_reload() {
  (void) load_syscalls();
}

bool B::are_syscalls_loaded() {
  static const i32 code = load_syscalls();
  return (code == 1);
}

B::_SyscallLoader::_SyscallLoader() {
  const bool R = are_syscalls_loaded();
  if __expect_false(!R) {
    /// Would rather have it panic, but since io isn't loaded,
    /// we obviously can't print here... 
    __builtin_trap();
  }
}
