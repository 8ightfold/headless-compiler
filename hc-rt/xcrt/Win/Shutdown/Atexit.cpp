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

struct AtexitTable {
  using ValueType = AtexitHandler*;
  constexpr AtexitTable() : pos(tbl) {}

  int push(AtexitHandler* handler) {
    if __expect_false(size() == 0)
      return -1;
    *pos++ = handler;
    return 0;
  }

  void runAllAndClear() {
    for (auto**& I = this->pos; I > begin(); --I) {
      AtexitHandler* const handler = *(I - 1);
      if (handler) {
        // TODO: Add try catch.
        handler();
      }
    }
  }

public:
  const ValueType* begin() const { return tbl; }
  const ValueType* end() const { return tbl + atexitTrueCount; }
  usize size() const { return (end() - pos); }
  bool isEmpty() const { return (pos == begin()); }
public:
  ValueType* pos = nullptr;
  ValueType  tbl[atexitTrueCount] {};
};

constinit AtexitTable atexitTbl {};
constinit AtexitTable quickexitTbl {};

static int addHandler(AtexitTable& tbl, AtexitHandler* handler) {
  if __expect_false(handler == nullptr)
    return 0;
  $XCRTLock(Atexit);
  return tbl.push(handler);
}

static void runHandlers(AtexitTable& tbl) {
  $XCRTLock(Atexit);
  // Table was either never initialized? or has already been cleared.
  // Either way, no point doing anything.
  if __likely_false(tbl.isEmpty())
    return;
  tbl.runAllAndClear();
}

} // namespace `anonymous`

usize xcrt::atexit_total() noexcept {
  return atexitTrueCount;
}

usize xcrt::atexit_slots() noexcept {
  return atexitTbl.size();
}

extern "C" {

int atexit(AtexitHandler* handler) noexcept {
  return addHandler(atexitTbl, handler);
}

int at_quick_exit(AtexitHandler* handler) noexcept {
  return addHandler(quickexitTbl, handler);
}

//////////////////////////////////////////////////////////////////////////

i32 __xcrt_atexit(AtexitHandler* handler) {
  return addHandler(atexitTbl, handler);
}

void __xcrt_invoke_atexit(void) {
  runHandlers(atexitTbl);
}

void __xcrt_invoke_quickexit(void) {
  runHandlers(quickexitTbl);
}

} // extern "C"
