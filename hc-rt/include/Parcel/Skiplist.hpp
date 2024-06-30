//===- Parcel/Skiplist.hpp ------------------------------------------===//
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
//  A colony-style skiplist with a statically sized array backing.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Limits.hpp>
#include <Common/Option.hpp>
#include <Common/RawLazy.hpp>
#include "BitList.hpp"

HC_HAS_BUILTIN(ffsll);

namespace hc::parcel {
  template <typename T, usize N>
  struct Skiplist;

  template <typename T, typename SKType>
  struct SkiplistHandle {
    constexpr SkiplistHandle(T* data, SKType* list)
     : __data(data), __list(list) { }
    
    constexpr SkiplistHandle(SkiplistHandle&& H)
     : __data(H.__data), __list(H.__list) {
      H.__data = nullptr;
    }

    constexpr ~SkiplistHandle() {
      this->erase();
    }

    constexpr bool erase() {
      const bool R = __list->eraseRaw(__data);
      this->__data = nullptr;
      return R;
    }

  public:
    T* __data;
    SKType* __list;
  };

  /// Uses a `BitList` to manage `RawLazy<T>` instances.
  /// Probably not the best choice for massive blocks.
  template <typename T, usize N>
  struct Skiplist {
    using SelfType = Skiplist;
    using DataType = com::RawLazy<T>;
    using Type = T;
    using BitType = uptr;
    using HandleType = SkiplistHandle<T, SelfType>;
    static constexpr auto __bSize = __bitsizeof(BitType);
    static constexpr auto __full  = Max<BitType>;
  public:
    constexpr ~Skiplist() {
      for (BitType I = 0; I < N; ++I) {
        if (__bits.get(I))
          __data[I].dtor();
      }
    }

    static constexpr usize FindEmptySlot(usize S) {
      return (S != 0) ? (__builtin_ffsll(~S) - 1) : 0;
    }

    [[nodiscard]] constexpr HandleType insert(auto&&...args) {
      return {insertRaw(__hc_fwd(args)...), this};
    }

    [[nodiscard]] constexpr T* insertRaw(auto&&...args) {
      auto& B = __bits.__uData();
      for (usize I = 0; I < __bits.__USize(); ++I) {
        if (B[I] == Max<BitType>)
          continue;
        const usize V = (I * __bSize) + FindEmptySlot(B[I]);
        __data[V].ctor(__hc_fwd(args)...);
        __bits[V] = true;
        return __data[V].data();
      }
      return nullptr;
    }

    constexpr bool eraseRaw(T* P) {
      if __expect_false(P == nullptr)
        return false;
      __hc_invariant(inRange(P));
      const auto V = P - __begin();
      if __expect_false(!__bits.get(V))
        return false;
      __bits[V] = false;
      __data[V].dtor();
      return true;
    }

    constexpr bool inRange(T* P) const {
      return P >= __begin() && P < __end();
    }

    constexpr const T* __begin() const {
      return __data[0].data();
    }

    constexpr const T* __end() const {
      return __data[0].data() + N;
    }

    /// Returns the amount of initialized elements.
    constexpr usize countActive() const {
      return __bits.countActive();
    }

    /// Returns the amount of uninitialized elements.
    constexpr usize countInactive() const {
      return __bits.countInactive();
    }

  public:
    DataType __data[N] {};
    BitList<N, BitType> __bits;
  };

  template <typename T, usize N>
  using ALSkiplist = Skiplist<T, com::Align::UpEq(N)>;
} // namespace hc::parcel
