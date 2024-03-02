//===- Phase1/Emutls.cpp --------------------------------------------===//
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
//  Based on the LLVM implementation of glibc's Emulated TLS.
//
//===----------------------------------------------------------------===//

#include <Common/Fundamental.hpp>
#include <Common/InlineMemcpy.hpp>
#include <Common/InlineMemset.hpp>
#include <Common/Limits.hpp>
#include <Common/RawLazy.hpp>
#include <Meta/Once.hpp>
#include <Sys/Core/Generic.hpp>
#include <Sys/Mutex.hpp>
#include <Mingw/TryPrint.hpp>
#include "Initialization.hpp"

using namespace hc;
using namespace xcrt;
namespace C = hc::common;
namespace win = hc::sys::win;

using ErrCode = win::DWord;
using TLSType = win::DWord;

extern "C" {
  void __xcrt_abort(void) throw() {
    _XCRT_DBGPRINT("__hc_abort() was called.");
  }
} // extern "C"

namespace {
  constexpr TLSType tls_out_of_indexes = Max<TLSType>;
  // TODO: Make RawLazy<sys::Mtx[]>
  constinit C::RawLazy<sys::Mtx> mtx {};
  constinit TLSType tls_idx = tls_out_of_indexes;
} // namespace `anonymous`

namespace xcrt::emutils {
  struct AddrArray {
    uptr skip_dtor_rounds;
    uptr size;
    void* data[] __counted_by(size);
  };

  static void shutdown(AddrArray* A);

  static inline void win_err(ErrCode code, const char* hint) {
    _XCRT_TRYPRINT("Error: Code %u (%s)\n", code, hint ? hint : "null");
  }

  static inline void win_abort(ErrCode code, const char* hint) {
    win_err(code, hint);
    __xcrt_abort();
  }

  static /*inline*/ void* maligned_alloc(usize align, usize size);
  static /*inline*/ void* maligned_free(void* base);

  static void emutils_exit() {

  }
} // namespace hcrt::emutils

extern "C" {
  void __xcrt_emutils_setup(void) {
    mtx.ctor();
    mtx->initialize();
  }

  void __xcrt_emutils_shutdown(void) {
    mtx.dtor();
  }
}