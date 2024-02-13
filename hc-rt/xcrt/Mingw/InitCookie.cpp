//===- InitCookie.cpp -----------------------------------------------===//
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
//  Initializes the Windows CRT security cookie.
//
//===----------------------------------------------------------------===//

#include "Startup.hpp"
#include <Bootstrap/Win64KernelDefs.hpp>

using namespace hc::bootstrap;

static constexpr uptr __hc_dsec_cookie_ = 0x00002B992DDFA232ll;

extern "C" {
  __attribute__((selectany))
  constinit uptr __security_cookie = __hc_dsec_cookie_;

  __attribute__((selectany))
  constinit uptr __security_cookie_complement = ~__hc_dsec_cookie_;

  void __security_init_cookie(void) {
    if (__security_cookie != __hc_dsec_cookie_) {
      __security_cookie_complement = ~__security_cookie;
      return;
    }

    auto* TEB = Win64TEB::LoadTEBFromGS();
    uptr cookie = __builtin_ia32_rdtsc();
    cookie ^= TEB->getProcessId();
    cookie ^= TEB->getThreadId();

    cookie &= 0x0000FFFFFFFFFFFFLL;
    if (cookie == __hc_dsec_cookie_)
      cookie = __hc_dsec_cookie_ + 1;
    __security_cookie = cookie;
    __security_cookie_complement = ~cookie;
  }
}
