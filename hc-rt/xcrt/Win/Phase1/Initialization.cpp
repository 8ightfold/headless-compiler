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
#include <Phase1/ArgParser.hpp>
#include <Bootstrap/_NtModule.hpp>
#include <Parcel/StaticVec.hpp>
#include <Sys/Args.hpp>
#include <xcrtDefs.hpp>
#include <String/Utils.hpp>
// TODO: Swap out
#include <GlobalXtors.hpp>

using namespace xcrt;
using namespace hc;
using namespace hc::bootstrap;

extern "C" {
/// C++ Setup function, in Phase0/Xtors.cpp
extern void __main(void);
extern int main(int argc, char** argv, char** envp);

constinit char* _acmdln = nullptr;
} // extern "C"

static void initArgv(PtrRange<char*> argv) {
  __hc_assert(_acmdln != nullptr);
  char* cmdln = _acmdln;
  for (char*& arg : argv) {
    const usize off = xcrt::stringlen(cmdln);
    arg = cmdln;
    cmdln += (off + 1);
    if __expect_false(*cmdln == '\0')
      break;
  }
}

[[gnu::noinline]] static int xcrtMainInvoker() {
  // TODO: Setup main here!!
  __main();
  pcl::StaticVec<char, RT_MAX_PATH + 1> tmp {};
  PtrRange<char*> argv;
  {
    auto exedir = sys::Args::ProgramDir();
    tmp.resizeUninit(exedir.size());
    Mem::Copy(tmp.data(), exedir.data(), exedir.size());
  }
  {
    auto wcmd = HcCurrentPEB()->process_params->commandline;
    const usize base_size = wcmd.getSize();
    DynAllocation<char> fullCommandline = $dynalloc(wcmd.getSize() + 2, char);

    const usize argCount = __setup_cmdline(
      fullCommandline.intoRange(), wcmd);
    _acmdln = fullCommandline.release();

    DynAllocation<char*> argvtmp = $zdynalloc(argCount + 2, char*);
    argv = argvtmp.intoRange().takeFront(argCount + 1);
    (void) argvtmp.release();
    initArgv(argv);
  }

  char* filler[] {nullptr};
  int ret = main(int(argv.size()), argv.data(), filler);
  // Destroy scoped statics, then destroy globals.
  __xcrt_invoke_atexit();
  return ret;
}

extern "C" {


void __xcrt_setup(void) {
  // Make sure syscalls are bootstrapped.
  force_syscall_reload();
  if (!are_syscalls_loaded()) {
    // TODO: Abort with message.
    __hc_unreachable("Fuck!");
  }

  // Set up CRT locks.
  __xcrt_locks_setup();

  // Init ANSI program/working directories.
  sys::__init_paths();

  // Set up standard IO.
  // TODO: __xcrt_sysio_setup();

  // Set up thread_local backend after sysio, 
  // as that may print on error.
  EMUTLS_STARTUP();

  // TODO: Set up TLS fr
  // TODO: Autorelocation >> _pei386_runtime_relocator
  // TODO: Set up SEH exception filter
}

void __xcrt_shutdown(void) {
  // Run shutdown functions in reverse order.
  EMUTLS_SHUTDOWN();
  // TODO: __xcrt_sysio_shutdown();
  __xcrt_locks_shutdown();
}

/// At this point, constructors still have not been called.
/// We need to get everything initialized, especially the syscalls.
/// Assume this is pure C++ (not C++/CLI), so no `__native_startup_state`.
[[gnu::used, gnu::noinline]]
int __xcrtCRTStartupPhase1(void) {
  __xcrt_setup();
  int ret = xcrtMainInvoker();
  __xcrt_shutdown();
  return ret;
}

} // extern "C"
