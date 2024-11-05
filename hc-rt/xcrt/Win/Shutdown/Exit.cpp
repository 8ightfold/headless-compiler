//===- Shutdown/Exit.cpp --------------------------------------------===//
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
#include <xcrt/Stdlib.hpp>
#include <Bootstrap/_NtModule.hpp>
#include <Bootstrap/KUserSharedData.hpp>
#include <Common/StrRef.hpp>
#include <Meta/ASM.hpp>
#include <Sys/Shutdown.hpp>
#include <Sys/Win/Except.hpp>

#define HC_FORCE_INTEL _ASM_NOPREFIX

using namespace hc;
using namespace hc::bootstrap;
using namespace hc::sys::win;

namespace {

[[gnu::noinline]] void printDbgMsg(StrRef S) {
  if (!ExceptionRecord::CheckDbgStatus())
    return;
  if (HcCurrentTEB()->in_debug_print)
    return;
  HcCurrentTEB()->in_debug_print = true;

  sys::RaiseException(
    0x40010006,
    S.size() + 2, S.data()
  );

  HcCurrentTEB()->in_debug_print = false;
}

[[gnu::noinline]] bool queryProcessorFeature(DWord feature) {
  if (feature >= eProcessorFeatureMax)
    return false;
  return !!KUSER_SHARED_DATA.ProcessorFeatures[feature];
}

} // namespace `anonymous`

extern "C" {

[[noreturn]] void abort() noexcept {
  // TODO: Split off dbgMsg/feature checks
#if _HC_DEBUG
  printDbgMsg("abort() has been called.");
#else
  if __expect_true(queryProcessorFeature(/*PF_FASTFAIL_AVAILABLE*/ 23)) {
    __asm__ volatile(
      ".intel_syntax noprefix;\n"
      // FAST_FAIL_FATAL_APP_EXIT
      "mov rcx, 55;\n"
      "int 41;\n" ::
    );
  }
#endif
  _Exit(3); // Same as the SIGABRT handler.
}

[[noreturn]] void _Exit(int status) noexcept {
  // TODO: __xcrt_sysio_shutdown();
  sys::exit(status);
}

[[noreturn]] void exit(int status) noexcept {
  // TODO: Unset TLS fr
  EMUTLS_SHUTDOWN();
  __xcrt_invoke_atexit();
  _Exit(status);
}

[[noreturn]] void quick_exit(int status) noexcept {
  __xcrt_invoke_quickexit();
  _Exit(status);
}

} // extern "C"
