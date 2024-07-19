//===- Sys/OSMutex.hpp ----------------------------------------------===//
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
//  A wrapper around platform-specific synchronization mechanisms.
//  If you want a simpler (and possibly faster under low contention)
//  underlying implementation, use AtomicMtx (unimplemented).
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Fundamental.hpp>
#include <Common/Limits.hpp>
#include <Common/PtrUnion.hpp>
#include "Locks.hpp"

// For more info:
// https://doxygen.reactos.org/de/db1/Mutex_8h_source.html
// https://github.com/llvm/llvm-project/blob/main/libc/src/__support/threads/linux/mutex.h

namespace hc::sys {
  union RawMtxHandle {
#  if HC_PLATFORM_WIN64
    using DefaultType = void*;
    static constexpr DefaultType npos = nullptr;
    constexpr RawMtxHandle() : __ptr(npos) { }
#  else
    using DefaultType = uptr;
    static constexpr DefaultType npos = Max<uptr>;
    constexpr RawMtxHandle() : __int(npos) { }
#  endif
    constexpr RawMtxHandle(void* P) : __ptr(P) { }
    constexpr RawMtxHandle(uptr  I) : __int(I) { }
  public:
    static RawMtxHandle New(const char* name = nullptr);
    static RawMtxHandle New(const wchar_t* name);
    static void Delete(RawMtxHandle H);
    static void Lock(RawMtxHandle H);
    static void LockMs(RawMtxHandle H, 
      usize ms = Max<usize>, bool X = false);
    static i32 Unlock(RawMtxHandle H);

    __always_inline DefaultType getUnderlying() const {
#  if HC_PLATFORM_WIN64
    return this->__ptr;
#  else
    return this->__int;
#  endif
    }

    bool isInitialized() const {
      return getUnderlying() != npos;
    }

  public:
    void* __ptr;
    uptr  __int;
  };

  struct OSMtx {
    static constexpr auto npos = RawMtxHandle::npos;
  public:
    constexpr OSMtx() = default;
    OSMtx(common::DualString S) noexcept : OSMtx() {
      this->initialize(S);
    }

    OSMtx(const OSMtx&) = delete;
    OSMtx& operator=(const OSMtx&) = delete;

    ~OSMtx() {
      if __expect_true(*this) {
        this->lock();
        RawMtxHandle::Delete(__data);
      }
    }

  public:
    void initialize() {
      if __expect_true(__data.isInitialized())
        return;
      this->__data = RawMtxHandle::New();
    }

    void initialize(common::DualString S) {
      if __expect_true(__data.isInitialized())
        return;
      S.visit([this] (auto* name) {
        this->__data = 
          RawMtxHandle::New(name);
      });
    }

    void lock() {
      if __expect_false(!*this)
        this->initialize();
      __locked = true;
      RawMtxHandle::Lock(__data);
      __lock_count += 1;
    }

    void unlock() {
      __lock_count -= 1;
      if (__lock_count <= 0) {
        __lock_count = 0;
        __locked = false;
      }
      RawMtxHandle::Unlock(__data);
    }

    __ndbg_inline explicit operator bool() const {
      return __data.isInitialized();
    }

    bool isLocked() const {
      return __locked;
    }

    __always_inline auto getUnderlying() const {
      return __data.getUnderlying();
    }

  public:
    RawMtxHandle __data {};
    u32  __lock_count = 0;
    bool __locked = false;
  };
} // namespace hc::sys
