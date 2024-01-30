//===- Common/PtrRange.hpp ------------------------------------------===//
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

#include "Features.hpp"
#include "Fundamental.hpp"

namespace hc::common {
  struct [[gsl::Pointer]] _VoidPtrProxy {
    using Type = _VoidPtrProxy;
    using Ref = Type&;
    using CRef = const Type&;
    using RawVoidType = void*;
    using TrueType = u8*;
  public:
    _VoidPtrProxy() = default;
    _VoidPtrProxy(const _VoidPtrProxy&) = default;
    _VoidPtrProxy(_VoidPtrProxy&&) = default;
    _VoidPtrProxy(auto* data) : __data((TrueType)data) { }
    constexpr _VoidPtrProxy(nullptr_t) : __data(nullptr) { }

    _VoidPtrProxy& operator=(const _VoidPtrProxy&) = default;
    _VoidPtrProxy& operator=(_VoidPtrProxy&&) = default;
    _VoidPtrProxy& operator=(auto* data) {
      this->__data = (TrueType)data;
      return *this;
    }
    _VoidPtrProxy& operator=(nullptr_t) {
      this->__data = nullptr;
      return *this;
    }

  private:
    template <typename R, typename...Args>
    _VoidPtrProxy(R(*)(Args...)) = delete;

    template <typename R, class Obj, typename...Args>
    _VoidPtrProxy(R(Obj::*)(Args...)) = delete;

  public:
    template <typename T>
    requires(!__is_void(T))
    explicit operator T*() const __noexcept {
      return reinterpret_cast<T*>(this->__data);
    } 

    operator RawVoidType() const __noexcept {
      return static_cast<RawVoidType>(this->__data);
    }

    [[gnu::always_inline]] 
    constexpr bool isEmpty() const {
      return this->__data == nullptr;
    }

    //=== Arithmetic Operators ===//

    template <typename T>
    requires(__is_integral(T))
    friend Type operator+(const Type& lhs, T rhs) {
      __hc_invariant(!lhs.isEmpty() || rhs == 0);
      return { lhs.__data + rhs };
    }

    template <typename T>
    requires(__is_integral(T))
    friend Type operator-(const Type& lhs, T rhs) {
      __hc_invariant(!lhs.isEmpty() || rhs == 0);
      return { lhs.__data - rhs };
    }

    friend uptrdiff operator-(const Type& lhs, auto* rhs) {
      __hc_invariant(!lhs.isEmpty() || rhs == nullptr);
      return lhs.__data - TrueType(rhs);
    }

    friend uptrdiff operator-(auto* lhs, const Type& rhs) {
      __hc_invariant(lhs != nullptr || rhs.isEmpty());
      return TrueType(lhs) - rhs.__data;
    }

    friend uptrdiff operator-(const Type& lhs, const Type& rhs) {
      __hc_invariant(!lhs.isEmpty() || rhs.isEmpty());
      return lhs.__data - rhs.__data;
    }

    friend constexpr bool operator==(
     const Type&, const Type&) = default;

  private:
    __prefer_type(void*) TrueType __data = nullptr;
  };

  template <typename T>
  struct _PtrRangeType {
    using type = T*;
  };

  template <>
  struct _PtrRangeType<void> {
    using type = _VoidPtrProxy;
  };

  template <typename T>
  using __ptr_range_base_t = typename
    _PtrRangeType<T>::type;

  template <typename T = void>
  struct PtrRange {
    using SelfType = PtrRange<T>;
    using PointerType = __ptr_range_base_t<T>;
  public:
    template <typename U = T>
    [[gnu::always_inline, gnu::const]]
    static PtrRange<U> New(U* begin, U* end) {
      return { begin, end };
    }

    [[gnu::always_inline, gnu::const]]
    static PtrRange<void> New(
     _VoidPtrProxy begin, _VoidPtrProxy end) {
      return { begin, end };
    }

    template <typename U = T>
    [[gnu::always_inline, gnu::const]]
    static PtrRange<U> New(U* begin, usize size) {
      if constexpr(__is_void(U)) {
        PointerType begin_adaptor { begin };
        return { begin_adaptor, begin_adaptor + size };
      } else {
        return { begin, begin + size };
      }
    }

    [[gnu::always_inline, gnu::const]]
    static PtrRange<void> New(
     _VoidPtrProxy begin, usize size) {
      return { begin, begin + size };
    }

    template <typename U = T, usize N>
    [[gnu::always_inline, gnu::const]]
    static PtrRange<U> New(U(&arr)[N])
     requires(!__is_void(U)) {
      return { arr, arr + N };
    }

    //=== Observers ===//

    __always_inline PtrRange& checkInvariants() {
      __hc_assert(__begin <= __end);
      return *this;
    }

    const PtrRange& checkInvariants() const {
      __hc_assert(__begin <= __end);
      return *this;
    }

    bool inRange(T* ptr) const {
      return (ptr >= __begin) && (ptr < __end);
    }

    usize size() const {
      return __end - __begin;
    }

    usize sizeInBytes() const {
      if constexpr(__is_void(T)) {
        return this->size();
      } else {
        return (__end - __begin) * __sizeof(T);
      }
    }

    T* data() const __noexcept {
      return this->__begin;
    }

    template <typename U = T>
    requires(!__is_void(U))
    const U& operator[](usize n) const {
      __hc_invariant(__begin != nullptr);
      __hc_invariant(n < this->size());
      return __begin[n];
    }

    T* begin() const { return this->__begin; }
    T* end() const { return this->__end; }

    SelfType slice(usize pos, usize n) const {
      __hc_invariant(__begin && (pos + n) <= size());
      return SelfType::New(__begin + pos, n);
    }
    
    SelfType slice(usize n) const {
      __hc_invariant(__begin && n <= size());
      return SelfType::New(__begin + n, __end);
    }

    SelfType dropFront(usize n = 1) const {
      return slice(n);
    }

    SelfType dropBack(usize n = 1) const {
      __hc_invariant(n <= size());
      return slice(0, size() - n);
    }

    SelfType takeFront(usize n = 1) const {
      if(n >= size()) return *this;
      return dropBack(size() - n);
    }

    SelfType takeBack(usize n = 1) const {
      if(n >= size()) return *this;
      return dropFront(size() - n);
    }

    template <typename U>
    requires(__is_void(T))
    __always_inline PtrRange<U> intoRange() const {
      const auto new_begin = (U*)__begin;
      const auto new_end   = (U*)__end;
      return { new_begin, new_end };
    }

    template <typename U>
    __always_inline U into() const {
      return U::New(__begin, __end);
    }

  public:
    PointerType __begin = nullptr;
    PointerType __end   = nullptr;
  };

  template <typename T>
  PtrRange(T*, T*) -> PtrRange<T>;

  using AddrRange = PtrRange<void>;
} // namespace hc::common
