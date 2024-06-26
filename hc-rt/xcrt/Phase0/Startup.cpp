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

#ifdef __has_attribute
# if __has_attribute(externally_visible)
#  define __externally_visible __attribute__((externally_visible))
# endif
#endif // __has_attribute?

#ifndef __externally_visible
# define __externally_visible
#endif

extern "C" {
  extern void __security_init_cookie(void);
  extern int  __xcrtCRTStartupPhase1(void);
} // extern "C"

extern "C" {
  [[gnu::force_align_arg_pointer]]
  __externally_visible int mainCRTStartup(void) {
    int __ret = 255;
    __security_init_cookie();
    __ret = __xcrtCRTStartupPhase1();
    return __ret;
  }
} // extern "C"