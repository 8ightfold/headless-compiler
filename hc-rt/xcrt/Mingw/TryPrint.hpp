//===- TryPrint.hpp -------------------------------------------------===//
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
//  Attempts to print to the debug stream.
//
//===----------------------------------------------------------------===//

#pragma once

#include "NtdllLdr.hpp"

#define _XCRT_TRYPRINT(fmt, args...) ((void) xcrt::__try_dbgprint(fmt, ##args))
#if _HC_DEBUG
# define _XCRT_DBGPRINT(fmt, args...) _XCRT_TRYPRINT(fmt, ##args)
#else
# define _XCRT_DBGPRINT(fmt, ...)
#endif

using DbgPrintType = u32(const char* fmt, ...);

namespace xcrt {
  [[gnu::noinline, gnu::cold]]
  u32 __try_dbgprint(const char fmt[], auto...vv) {
    auto* F = __load_ntdll_func<DbgPrintType>("DbgPrint");
    if __expect_false(!F) return -1;
    return F(fmt, vv...);
  }
} // namespace hcrt
