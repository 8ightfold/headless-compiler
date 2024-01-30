//===- BinaryFormat/Consumer.hpp ------------------------------------===//
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
//  This file defines an object which can be used to more easily walk
//  through a loaded binary.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Features.hpp>
#include <Common/Fundamental.hpp>
#include <Common/PtrRange.hpp>
#include "MagicMatcher.hpp"

namespace hc::binfmt {
  struct Consumer {
    Consumer() = default;
    Consumer(common::AddrRange image) : __image(image) { }
  public:
    static Consumer New(void* begin, void* end) {
      const common::AddrRange addrs { begin, end };
      return { addrs.checkInvariants() };
    }

    static Consumer New(common::AddrRange addrs) {
      return { addrs.checkInvariants() };
    }

    bool matches(MMagic chk) const __noexcept {
      if __expect_false(!chk) return false;
      MMagic m = MMagic::Match(__image);
      return chk.isEqual(m);
    }

    //=== Extractors ===//

    template <typename T>
    __always_inline T* into() const __noexcept {
      return static_cast<T*>(__image.data());
    }

    template <typename T>
    T* intoIf(bool cond) const __noexcept {
      if __unpredictable(cond) 
        return this->into<T>();
      return nullptr;
    }

    template <typename T>
    T* intoIfMatches(MMagic chk) const __noexcept {
      if(this->matches(chk)) 
        return this->into<T>();
      return nullptr;
    }

    template <typename T>
    __always_inline common::PtrRange<T>
     intoRange(usize n) const __noexcept {
      return this->intoRangeRaw<T>(__sizeof(T) * n);
    }

    template <typename T>
    common::PtrRange<T> intoRangeRaw(usize n) const __noexcept {
      return __image.takeFront(n).intoRange<T>();
    }

    //=== Consumers ===//

    template <typename T>
    __always_inline T* consume() __noexcept {
      const auto p = this->into<T>();
      this->__image = __image.dropFront(__sizeof(T));
      return p;
    }

    template <typename T>
    T* consumeIf(bool cond) __noexcept {
      if __unpredictable(cond) 
        return this->consume<T>();
      return nullptr;
    }

    template <typename T>
    T* consumeIfMatches(MMagic chk) __noexcept {
      if(this->matches(chk))
        return this->consume<T>();
      return nullptr;
    }

    template <typename T>
    __always_inline common::PtrRange<T> 
     consumeRange(usize n) __noexcept {
      return this->consumeRangeRaw<T>(__sizeof(T) * n);
    }

    template <typename T>
    common::PtrRange<T> consumeRangeRaw(usize n) __noexcept {
      const auto range = this->intoRangeRaw<T>(n);
      this->__image = __image.dropFront(n);
      return range;
    }

    //=== Consume with Subtraction ===//

    template <typename T, typename Int>
    __always_inline T* consumeAndSub(Int& i) __noexcept
     requires(__is_integral(Int)) {
      i -= sizeof(T);
      return this->consume<T>();
    }

    template <typename T, typename Int>
    __always_inline T* consumeAndSubIf(bool cond, Int& i) __noexcept
     requires(__is_integral(Int)) {
      if __unpredictable(cond)
        return this->consumeAndSub<T>(i);
      return nullptr;
    }

    //=== Dropping ===//

    template <typename T>
    Consumer& drop(usize n) __noexcept {
      this->__image = __image.dropFront(__sizeof(T) * n);
      return *this;
    }

    Consumer& dropRaw(usize n) __noexcept {
      this->__image = __image.dropFront(n);
      return *this;
    }

  private:
    common::AddrRange __image = { };
  };
} // namespace hc::binfmt
