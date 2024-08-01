//===- Phase0/Xtors.cpp ---------------------------------------------===//
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

extern "C" {
static constinit bool __did_init_ctors_ = false;

[[gnu::used]] void __do_global_ctors(void) {
  if __expect_true(__did_init_ctors_)
    return;
  __did_init_ctors_ = true;

  auto N = reinterpret_cast<usize>(__CTOR_LIST__[0]);
  if (N == static_cast<usize>(-1)) {
    for (N = 0; __CTOR_LIST__[N + 1] != 0; ++N);
  }
  for (usize I = N; I >= 1; --I)
    __CTOR_LIST__[I]();
  // TODO: Set atexit(__do_global_dtors)
}

[[gnu::used]] void __do_global_dtors(void) {
  static bool __did_fini = false;
  if __expect_false(__did_fini)
    return;
  __did_fini = true;

  const XtorFunc* D = __DTOR_LIST__;
  while (*++D) { (*D)(); }
}

[[gnu::used]] void __main(void) {
  if __expect_true(__did_init_ctors_)
    return;
  __do_global_ctors();
}
} // extern "C"
