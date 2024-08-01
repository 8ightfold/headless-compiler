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
#include <Parcel/StaticVec.hpp>
#include <Sys/Args.hpp>
#include <xcrtDefs.hpp>
// TODO: Swap out
#include <GlobalXtors.hpp>

using namespace hc;
using namespace hc::bootstrap;

#if _HC_EMUTLS
# define EMUTLS_STARTUP()   __xcrt_emutils_setup()
# define EMUTLS_SHUTDOWN()  __xcrt_emutils_shutdown()
#else
# define EMUTLS_STARTUP()   (void(0))
# define EMUTLS_SHUTDOWN()  (void(0))
#endif

extern "C" {
/// C++ Setup function, in Phase0/Xtors.cpp
extern void __main(void);
extern int main(int argc, char** argv, char** envp);
} // extern "C"

static int xcrtMainInvoker() {
  // TODO: Setup main here!!
  __main();
  pcl::StaticVec<char, RT_MAX_PATH + 1> tmp {};
  for (wchar_t C : sys::Args::WorkingDir())
    tmp.push(static_cast<char>(C));
  tmp.push('\0');
  char* filler[] {tmp.data(), nullptr};
  int ret = main(1, filler, filler);
  // TODO: Swap out
  __do_global_dtors();
  return ret;
}

extern "C" {
/// At this point, constructors still have not been called.
/// We need to get everything initialized, especially the syscalls.
/// Assume this is pure C++ (not C++/CLI), so no `__native_startup_state`.
[[gnu::used, gnu::noinline]]
int __xcrtCRTStartupPhase1(void) {
  // Make sure syscalls are bootstrapped.
  force_syscall_reload();
  if (!are_syscalls_loaded()) {
    // TODO: Abort with message.
    __hc_unreachable("Fuck!");
  }
  // Set up CRT locks.
  __xcrt_locks_setup();
  // __xcrt_sysio_setup();
  // Set up thread_local backend after sysio, 
  // as that may print on error.
  EMUTLS_STARTUP();

  // TODO: Set up TLS fr
  // TODO: Autorelocation >> _pei386_runtime_relocator
  // TODO: Set up SEH exception filter
  int ret = xcrtMainInvoker();

  // Run shutdown functions in reverse order.
  EMUTLS_SHUTDOWN();
  // __xcrt_sysio_shutdown();
  // __xcrt_locks_shutdown();

  return ret;
}
} // extern "C"
