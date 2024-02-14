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
#include <Common/PtrUnion.hpp>

#if HC_PLATFORM_WIN64
# define __hc_mtxh(m...) ((m).__ptr)
#else
# define __hc_mtxh(m...) ((m).__int)
#endif

// TODO: Make atomic or sumn
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
    static constexpr DefaultType npos = -1;
    constexpr RawMtxHandle() : __int(npos) { }
#  endif
    constexpr RawMtxHandle(void* P) : __ptr(P) { }
    constexpr RawMtxHandle(uptr  I) : __int(I) { }
  public:
    static RawMtxHandle New(const char* name = nullptr);
    static RawMtxHandle New(const wchar_t* name);
    static void Delete(RawMtxHandle H);
    static void Lock(RawMtxHandle H);
    // TODO: Add `common::Max<usize>` !?
    static void LockMs(RawMtxHandle H, usize ms, bool /*alert*/ = false);
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

  struct Mtx {
    static constexpr auto npos = RawMtxHandle::npos;
  public:
    constexpr Mtx() = default;
    Mtx(common::DualString S) noexcept : Mtx() {
      this->initialize(S);
    }

    Mtx(const Mtx&) = delete;
    Mtx& operator=(const Mtx&) = delete;

    ~Mtx() {
      this->lock();
      RawMtxHandle::Delete(__data);
    }

  public:
    void initialize() {
      this->__data = RawMtxHandle::New();
    }

    void initialize(common::DualString S) {
      S.visit([this] (auto* name) {
        this->__data = 
          RawMtxHandle::New(name);
      });
    }

    void lock() {
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

    bool isLocked() const {
      return __locked;
    }

  public:
    RawMtxHandle __data {};
    u32  __lock_count = 0;
    bool __locked = false;
  };

  template <typename MutexType>
  struct ScopedLock {
    using MtxType = MutexType;
  public:
    __always_inline 
     ScopedLock(MutexType& mtx) 
     : __mtx(mtx) {
      __mtx.lock();
    }

    ScopedLock(const ScopedLock&) = delete;
    ScopedLock& operator=(const ScopedLock&) = delete;

    ~ScopedLock() {
      __mtx.unlock();
    }

  private:
    MutexType& __mtx;
  };

  __ndbg_inline auto* __get_mtx_handle(Mtx& M) {
    return &__hc_mtxh(M.__data);
  }
} // namespace hc::sys
