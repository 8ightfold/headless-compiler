//===- GccMain.cpp --------------------------------------------------===//
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

#include "GlobalXtors.hpp"

extern "C" {
  [[gnu::used]] void __do_global_ctors(void) {
    static bool __did_init = false;
    if (__builtin_expect(__did_init, 0))
      return;
    __did_init = true;

    auto N = reinterpret_cast<unsigned long long>(__CTOR_LIST__[0]);
    if (N == static_cast<unsigned long long>(-1)) {
      for (N = 0; __CTOR_LIST__[N + 1] != 0; ++N);
    }
    for (unsigned I = (unsigned)N; I >= 1; --I)
      __CTOR_LIST__[I]();
  }

  [[gnu::used]] void __do_global_dtors(void) {
    static bool __did_fini = false;
    if (__builtin_expect(__did_fini, 0))
      return;
    __did_fini = true;

    const XtorFunc* D = __DTOR_LIST__ + 1;
    while (*D) {
      (*D)();
      ++D;
    }
  }

  [[gnu::used]] void __main(void) {
    static bool __did_init = false;
    if (!__did_init) {
      __did_init = true;
      __do_global_ctors();
    } 
  }
} // extern "C"
