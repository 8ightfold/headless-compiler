//===- Sys/Mutex.hpp ------------------------------------------------===//
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

#pragma once

#include <Common/Fundamental.hpp>

#if HC_PLATFORM_WIN64
# define __hc_mtxh(m...) ((m).__ptr)
#else
# define __hc_mtxh(m...) ((m).__int)
#endif

namespace hc::sys {
  union RawMtxHandle {
#  if HC_PLATFORM_WIN64
    constexpr RawMtxHandle() : __ptr(nullptr) { }
#  else
    constexpr RawMutexHandle() : __int(0) { }
#  endif
    constexpr RawMtxHandle(void* P) : __ptr(P) { }
    constexpr RawMtxHandle(uptr  I) : __int(I) { }
  public:
    void* __ptr;
    uptr  __int;
  };

  struct Mtx {
  public:
    RawMtxHandle __data {};
  };

  __ndbg_inline auto* __get_mtx_handle(Mtx& M) {
    return &__hc_mtxh(M.__data);
  }
} // namespace hc::sys
