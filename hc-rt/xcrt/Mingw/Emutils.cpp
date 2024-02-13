//===- Emutils.cpp --------------------------------------------------===//
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
//  This file implements the underlying TLS API.
//  Based on the LLVM implementation of glibc's emutils.
//
//===----------------------------------------------------------------===//

#include <Common/Fundamental.hpp>
#include <Common/InlineMemcpy.hpp>
#include <Common/InlineMemset.hpp>
#include "TryPrint.hpp"

extern "C" {
  void __xcrt_abort(void) throw() {
    _XCRT_DBGPRINT("__hc_abort() was called.");
  }
} // extern "C"

namespace xcrt::emutils {
  struct AddrArray {
    uptr skip_dtor_rounds;
    uptr size;
    void* data[];
  };

  static void shutdown(AddrArray* A);

  [[gnu::cold]] static void win_err(u32 code, const char* hint) {
    _XCRT_TRYPRINT("Error: Code %u (%s)\n", code, hint ? hint : "null");
    __xcrt_abort();
  }
} // namespace hcrt

extern "C" {
  void __emutils_setup(void) {
    
  }
}