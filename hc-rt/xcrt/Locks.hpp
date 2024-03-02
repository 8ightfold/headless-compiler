//===- Locks.hpp ----------------------------------------------------===//
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
//  Implementation can be found in {PLATFORM}/Phase1/Locks.cpp.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Sys/Mutex.hpp>
#include <Sys/Locks.hpp>

#define $XCRTLock(value) \
 ::hc::sys::ScopedPtrLock $var(xcrt_lock) \
  {::__xcrt_get_lock(::xcrt::Locks::value)}

namespace xcrt {
  enum class Locks : u64 {
    ProcessInfoBlock,
    ThreadLocalStorage,
    AtExit,
    MaxValue
  };

  extern "C" hc::sys::Mtx*
    __xcrt_get_lock(xcrt::Locks V);
} // namespace xcrt