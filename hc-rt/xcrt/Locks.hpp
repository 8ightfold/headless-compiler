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

#define _XCRT_BASIC_LOCK 1

#if _XCRT_BASIC_LOCK
# include <Sys/Mutex.hpp>
#else
# include <Sys/OSMutex.hpp>
#endif
#include <Sys/Locks.hpp>

#define $XCRTLock(value) \
 ::hc::sys::ScopedLock $var(xcrt_lock) \
  {::xcrt::get_lock(::xcrt::Locks::value)}

#if defined(RT_MAX_THREADS) && (RT_MAX_THREADS != 0)
# error Multithreading temporarily disabled.
#endif

namespace xcrt {

enum class Locks : u64 {
  ProcessInfoBlock,
  ThreadLocalStorage,
  Atexit,
  MaxValue
};

#if _XCRT_BASIC_LOCK
using LockType = hc::sys::Mtx;
#else
using LockType = hc::sys::OSMtx;
#endif

LockType& get_lock(Locks V);

} // namespace xcrt

extern "C" xcrt::LockType*
  __xcrt_get_lock(xcrt::Locks V);
