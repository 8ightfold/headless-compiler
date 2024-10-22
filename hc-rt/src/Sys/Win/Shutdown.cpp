//===- Sys/Win/Shutdown.hpp -----------------------------------------===//
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
//  Low-level shutdown functions for a platform.
//
//===----------------------------------------------------------------===//

#include <Sys/Shutdown.hpp>
#include <Sys/Win/Process.hpp>
#include <Bootstrap/_NtModule.hpp>
#include <Common/DefaultFuncPtr.hpp>
#include <Meta/Unwrap.hpp>

#define $load_symbol(name) \
  succeeded &= load_nt_symbol(name, #name)

using namespace hc;
using namespace hc::sys::win;
using bootstrap::__NtModule;

namespace hc::sys {

using FnType = DefaultFuncPtr<void()>;
__imut FnType RtlAcquirePebLock {};
__imut FnType RtlReleasePebLock {};
__imut FnType LdrShutdownProcess {};

template <typename F>
static bool load_nt_symbol(
 DefaultFuncPtr<F>& func, StrRef symbol) {
  if __expect_true(func.isSet())
    return true;
  auto exp = __NtModule()->resolveExport<F>(symbol);
  return func.setSafe($unwrap(exp));
}

static bool init_exit() {
  static bool has_succeeded = false;
  if __expect_false(!has_succeeded) {
    bool succeeded = true;
    $load_symbol(RtlAcquirePebLock);
    $load_symbol(RtlReleasePebLock);
    $load_symbol(LdrShutdownProcess);
    has_succeeded = succeeded;
  }
  return has_succeeded;
}

} // namespace hc::sys

[[noreturn]] void hc::sys::exit(int status) {
  static bool has_exit = init_exit();
  if __expect_false(!has_exit) {
    // TODO: Log failure?
    sys::terminate(status);
    $unreachable_msg("Shit.");
  }

  RtlAcquirePebLock();
  terminate_process_threads(status);
  LdrShutdownProcess(); // Unload dlls
  // TODO: CsrClientCallServer
  terminate_process(status); // Terminate current process.

  RtlReleasePebLock(); // Should never get here...
  $unreachable_msg("exit call failed.");
}

[[noreturn]] void hc::sys::terminate(int status) {
  terminate_process(win::NtStatus(status));
  $unreachable_msg("terminate call failed.");
}
