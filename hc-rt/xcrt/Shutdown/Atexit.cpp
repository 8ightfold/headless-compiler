//===- Shutdown/Atexit.cpp ------------------------------------------===//
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

#include <Phase1/Initialization.hpp>
#include <xcrt/Stdlib.hpp>

#ifndef RT_MAX_ATEXIT
# error RT_MAX_ATEXIT must be defined!
#endif

namespace {

constexpr usize atexitCount = (RT_MAX_ATEXIT >= 16) ? RT_MAX_ATEXIT : 16;
constexpr usize atexitTrueCount = atexitCount + 2;
constinit AtexitHandler* atexitTbl[atexitTrueCount] {};

constexpr AtexitHandler** atexit_begin = atexitTbl;
constinit AtexitHandler** atexit_pos = atexitTbl;
constexpr AtexitHandler** atexit_end = atexitTbl + atexitTrueCount;

} // namespace `anonymous`

usize xcrt::atexit_total() noexcept {
  return atexitTrueCount;
}

usize xcrt::atexit_slots() noexcept {
  return static_cast<usize>(atexit_end - atexit_pos);
}

extern "C" {

int atexit(AtexitHandler* handler) noexcept {
  return __xcrt_atexit(handler);
}

int at_quick_exit(AtexitHandler* handler) noexcept {
  return -1;
}

//////////////////////////////////////////////////////////////////////////

i32 __xcrt_atexit(AtexitHandler* handler) {
  if __expect_false(handler == nullptr)
    return 0;
  
  $XCRTLock(Atexit);
  if __likely_false(atexit_pos == atexit_end)
    return -1;
  *atexit_pos++ = handler;
  return 0;
}

void __xcrt_invoke_atexit(void) {
  $XCRTLock(Atexit);
  // Table was either never initialized? or has already been cleared.
  // Either way, no point doing anything.
  if __likely_false(atexit_pos == atexit_begin)
    return;

  for (auto**& I = atexit_pos; I > atexit_begin; --I) {
    AtexitHandler* const handler = *(I - 1);
    if (handler) {
      // TODO: Add try catch.
      handler();
    }
  }
}

} // extern "C"
