//===- Phase1/Initialization.cpp ------------------------------------===//
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

#include <Phase1/Initialization.hpp>
#include <xcrtDefs.hpp>

using namespace hc::bootstrap;

extern "C" {
  /// At this point, constructors still have not been called.
  /// We need to get everything initialized, especially the syscalls.
  [[gnu::used, gnu::noinline]]
  int __xcrtCRTStartupPhase1(void) {
    // Make sure syscalls are bootstrapped.
    force_syscall_reload();
    if (!are_syscalls_loaded())
      // TODO: Abort with message.
      __hc_unreachable("Fuck!");
    // Set up CRT locks.
    __xcrt_locks_setup();
    __xcrt_sysio_setup();
    // Set up thread_local backend.
    // After sysio as that may print on error.
    __xcrt_emutils_setup();

    // TODO: Setup main here!!

    // Run shutdown functions in reverse order.
    __xcrt_emutils_shutdown();
    __xcrt_sysio_shutdown();
    __xcrt_locks_shutdown();

    return -1;
  }
}
