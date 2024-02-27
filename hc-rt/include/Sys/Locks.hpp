//===- Sys/Locks.hpp ------------------------------------------------===//
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

namespace hc::sys {
  template <typename MutexType>
  struct ScopedLock {
    using MtxType = MutexType;
  public:
    __always_inline 
     ScopedLock(MutexType& mtx) : 
     __mtx(mtx) {
      __mtx.lock();
    }

    ScopedLock(const ScopedLock&) = delete;
    ScopedLock(ScopedLock&&) = delete;
    ScopedLock& operator=(const ScopedLock&) = delete;
    ScopedLock& operator=(ScopedLock&&) = delete;

    __always_inline
     ~ScopedLock() {
      __mtx.unlock();
    }

  private:
    MutexType& __mtx;
  };
} // namespace hc::sys