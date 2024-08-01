//===- Common/AtomicLazy.hpp ----------------------------------------===//
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
//  Defines a wrapped RawLazy<T> with atomic initialization checks.
//  Only use for very simple cases with almost no contention.
//  This also carries over to usage, eg. calling dtor while holding a ref.
//
//===----------------------------------------------------------------===//

#pragma once

#include "RawLazy.hpp"
#include "Function.hpp"
#include <Sys/AtomicMutex.hpp>

namespace hc::common {
  template <typename T>
  struct AtomicLazy : RawLazy<T> {
    using enum sys::MemoryOrder;
    using Type = T;
    using SelfType = AtomicLazy<T>;
    using BaseType = RawLazy<T>;
    using BaseType::__data;
  protected:
    using BaseType::ctor;
    using BaseType::dtor;
  public:
    inline T& ctor(auto&&...args) noexcept {
      if (__init.load(Acquire))
        return BaseType::unwrap();
      __mtx.lock();
      T& ref = BaseType::ctor(__hc_fwd(args)...);
      __init.store(true, Release);
      __mtx.unlock();
      return ref;
    }

    inline T& ctorDeferred(Function<T&(BaseType&)> fn) noexcept {
      if __expect_true(__init.load(Acquire))
        return BaseType::unwrap();
      __mtx.lock();
      T& ref = fn(static_cast<BaseType&>(*this));
      __hc_invariant(__addressof(ref) == BaseType::data());
      __init.store(true, Release);
      __mtx.unlock();
      return ref;
    }

    inline void dtor() noexcept {
      if (!__init.load(Acquire))
        return;
      __mtx.lock();
      BaseType::dtor();
      __init.store(false, Release);
      __mtx.unlock();
    }

    __always_inline bool isFull() noexcept {
      return __init.load();
    }

    __always_inline bool isEmpty() noexcept {
      return !__init.load();
    }

  public:
    sys::Atomic<bool> __init {};
    sys::AtomicMtx    __mtx {};
  };
} // namespace hc::common
