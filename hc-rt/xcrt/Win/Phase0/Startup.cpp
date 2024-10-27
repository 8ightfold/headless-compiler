//===- Phase0/Startup.cpp -------------------------------------------===//
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

#include <GlobalXtors.hpp>
#include <Common/Fundamental.hpp>

#if defined(_WINMAIN_) || defined(WPRFLAG) || defined(_MANAGED_MAIN)
# error Invalid main signature! Only mainCRTStartup is supported.
#endif

#undef __externally_visible
#ifdef __has_attribute
# if __has_attribute(externally_visible)
#  define __externally_visible __attribute__((externally_visible))
# endif
#endif // __has_attribute?

#ifndef __externally_visible
# define __externally_visible __attribute__((used))
#endif

extern "C" {
// Phase0/InitCookie.cpp
extern void __security_init_cookie(void);
// Phase1/Initialization.cpp
extern int  __xcrtCRTStartupPhase1(void);
} // extern "C"

// TODO: Implement SEH
#undef __SEH__

extern "C" {
  [[gnu::force_align_arg_pointer]]
  __externally_visible int mainCRTStartup(void) {
    int __ret = 255;
#ifdef __SEH__
    __asm__ volatile (".l_start:;");
#endif
    __security_init_cookie();
    __ret = __xcrtCRTStartupPhase1();
#ifdef __SEH__
  __asm__ volatile (
    "nop;"
    ".l_end: nop;"
    ".seh_handler [C HANDLER], @except;"
    ".seh_handlerdata;"
    ".long 1;"
    ".rva .l_start, .l_end, [EXCEPTION HANDLER], .l_end;"
    ".text"
  );
#endif
    return __ret;
  }
} // extern "C"
