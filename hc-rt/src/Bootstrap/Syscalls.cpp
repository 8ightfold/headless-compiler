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
#include <Bootstrap/ModuleParser.hpp>
#include <Meta/Refl.hpp>
#include <Meta/Unwrap.hpp>

using namespace hc;
using namespace hc::bootstrap;
namespace B = hc::bootstrap;

using DbgType = ULong(const char* fmt, ...);

namespace hc::bootstrap {
constinit common::EnumArray<u32, Syscall> __syscalls_ {};
/// Defined in `StubParser.cpp`.
COFFModule& __NtModule();
} // namespace hc::bootstrap

namespace {
  static constinit EnumArray<StubError, Syscall> __errors_ {};
  static constinit bool __did_succeed_ = false;

  [[gnu::hot]] u32 handle_syscall(Syscall C, StrRef name) {
    StubResult stub = parse_stub(name);
    if __expect_false(stub.isErr()) {
      __errors_[C] = stub.err();
      __did_succeed_ = false;
      return __invalid_syscall_;
    }
    __errors_[C] = StubError::Success;
    return stub.ok();
  }

  [[gnu::cold]] bool load_syscalls() {
    if (__did_succeed_) return true;
    __did_succeed_ = true;
#  define $NtGen(name) __syscalls_[Syscall::name] = \
    handle_syscall(Syscall::name, $stringify(Nt##name));
#  include <Bootstrap/Syscalls.mac>
    return __did_succeed_;
  }

  [[gnu::cold]] void __list_invalid(DbgType& DbgPrint) {
    static constexpr auto& F     = $reflexpr(Syscall).Fields();
    static constexpr auto& Stubs = $reflexpr(StubError).Fields();
    DbgPrint("Unable to load the following syscalls:\n");
    for (usize I = 0; I < F.Count(); ++I) {
      const Syscall C = F[I];
      if (__syscalls_[C] != __invalid_syscall_)
        continue;
      DbgPrint("Nt%s [%s]\n", 
        F.Name(C), Stubs.Name(__errors_[C]));
    }
  }

  /// Attempts to load the debug print functions on error.
  /// It will then forward the function to `__list_invalid`.
  [[gnu::cold]] void __try_dbgprint() {
    auto& M = __NtModule();
    auto res = M.resolveExport<DbgType>("DbgPrint");
    if (res.isNone()) return;
    __list_invalid(res.some());
  }
} // namespace `anonymous`

namespace hc::bootstrap {
void force_syscall_reload() {
  __did_succeed_ = false;
  (void) load_syscalls();
}

bool are_syscalls_loaded() {
  return load_syscalls();
}

#ifndef __XCRT__
_SyscallLoader::_SyscallLoader() {
  const bool R = are_syscalls_loaded();
  if __expect_false(!R) {
    __try_dbgprint();
    /// Would rather have it always panic, but since io 
    /// might not be loaded, we obviously wouldn't be able to... 
    __hc_trap();
  }
}
#endif // __XCRT__?
} // namespace hc::bootstrap
