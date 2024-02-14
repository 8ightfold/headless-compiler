//===- Gnu/RtBegin.cpp ----------------------------------------------===//
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
  [[gnu::visibility("hidden")]]
  void* __dso_handle = nullptr;

  [[gnu::weak]]
  extern void __cxa_finalize(void*);

  [[gnu::section(".ctors"), gnu::aligned(8), gnu::used]]
  XtorFunc __CTOR_LIST__[] = {(XtorFunc)-1};
  
  [[gnu::section(".dtors"), gnu::aligned(8), gnu::used]]
  XtorFunc __DTOR_LIST__[] = {(XtorFunc)-1};

  extern XtorFunc __CTOR_END__[];
  extern XtorFunc __DTOR_END__[];

  [[gnu::used]] void __do_global_ctors(void) {
    static bool __did_init = false;
    if (__builtin_expect(__did_init, 0))
      return;
    __did_init = true;
    const unsigned N = __CTOR_END__ - __CTOR_LIST__ - 1;
    for (unsigned I = N; I >= 1; --I)
      __CTOR_LIST__[I]();
  }

  [[gnu::used]] void __do_global_dtors(void) {
    static bool __did_fini = false;
    if (__builtin_expect(__did_fini, 0))
      return;
    __did_fini = true;

    if (__cxa_finalize)
      __cxa_finalize(__dso_handle);

    const unsigned N = __DTOR_END__ - __DTOR_LIST__ - 1;
    for (unsigned I = 1; I <= N; ++I)
      __DTOR_LIST__[I]();
  }
} // extern "C"
