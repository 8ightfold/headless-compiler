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

#include <Meta/Traits.hpp>
#include <Meta/_QualTraits.hpp>

#define $PRange(ty...) ::hc::common::PtrRange<ty>

namespace hc::common {

template <typename RawVoid = void>
requires meta::is_void<meta::RemoveCVRef<RawVoid>>
struct [[gsl::Pointer]] _VoidPtrProxy {
  using Type = _VoidPtrProxy;
  using Ref = Type&;
  using CRef = const Type&;
  using RawVoidType = RawVoid*;

  template <typename T>
  using CastType = meta::__copy_quals<T, RawVoid>;
  using TrueType = CastType<u8>*;

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
  template <meta::not_void T>
  explicit operator T*() const __noexcept {
    static_assert(meta::__matching_quals<T, RawVoid>);
    return reinterpret_cast<T*>(this->__data);
  } 

  operator RawVoid*() const __noexcept {
    return static_cast<RawVoid*>(this->__data);
  }

  inline constexpr RawVoidType get() const {
    return static_cast<RawVoid*>(this->__data);;
  }

  [[gnu::always_inline]] 
  constexpr bool isEmpty() const {
    return this->__data == nullptr;
  }

  //==================================================================//
  // Arithmetic Operators
  //==================================================================//

  template <meta::is_integral T>
  friend Type operator+(const Type& lhs, T rhs) {
    __hc_invariant(!lhs.isEmpty() || rhs == 0);
    return { lhs.__data + rhs };
  }

  template <meta::is_integral T>
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
  TrueType __data = nullptr;
};

template <typename T>
_VoidPtrProxy(T*) -> _VoidPtrProxy<void>;

template <typename T>
_VoidPtrProxy(const T*) -> _VoidPtrProxy<const void>;

template <typename T>
_VoidPtrProxy(volatile T*) -> _VoidPtrProxy<volatile void>;

template <typename T>
_VoidPtrProxy(const volatile T*) -> _VoidPtrProxy<const volatile void>;

////////////////////////////////////////////////////////////////////////
// Traits

template <typename T>
struct _PtrRangeType {
  using type = T*;
};

template <typename RawVoid>
requires meta::is_void<meta::RemoveCVRef<RawVoid>>
struct _PtrRangeType<RawVoid> {
  using type = _VoidPtrProxy<RawVoid>;
};

template <typename T>
using __ptr_range_base_t = typename
  _PtrRangeType<T>::type;

template <typename T>
[[nodiscard]] inline constexpr usize ptr_distance(
 const T* lhs, const T* rhs) noexcept {
  __hc_invariant(lhs <= rhs);
  return (rhs - lhs);
}

[[nodiscard]] inline usize ptr_distance(
 const void* lhs, const void* rhs) noexcept {
  auto* blhs = static_cast<const u8*>(lhs);
  auto* brhs = static_cast<const u8*>(rhs);
  return ptr_distance(blhs, brhs);
}

template <typename VT>
[[nodiscard]] __always_inline usize ptr_distance(
 _VoidPtrProxy<VT> lhs, _VoidPtrProxy<VT> rhs) noexcept {
  return ptr_distance(lhs.get(), rhs.get());
}

template <typename T>
[[nodiscard]] inline constexpr usize ptr_distance_bytes(
 const T* lhs, const T* rhs) noexcept {
  const usize dist = ptr_distance(lhs, rhs);
  if constexpr (meta::not_void<T>) {
    return dist * sizeof(T);
  } else {
    return dist;
  }
}

template <typename VT>
[[nodiscard]] __always_inline usize ptr_distance_bytes(
 _VoidPtrProxy<VT> lhs, _VoidPtrProxy<VT> rhs) noexcept {
  return ptr_distance_bytes(lhs.get(), rhs.get());
}

////////////////////////////////////////////////////////////////////////
// Implementation

template <typename T = __dummy>
struct PtrRange;

template <> struct PtrRange<__dummy> {
  template <typename T>
  [[gnu::always_inline, gnu::const]]
  constexpr static PtrRange<T> New() {
    return { nullptr, 0 };
  }

  template <typename T>
  [[gnu::always_inline, gnu::const]]
  constexpr static PtrRange<T> New(T* begin, T* end) {
    using ProxyType = __ptr_range_base_t<T>;
    return { ProxyType(begin), ProxyType(end) };
  }

  template <typename VT>
  [[gnu::always_inline, gnu::const]]
  static PtrRange<VT> New(
   _VoidPtrProxy<VT> begin, _VoidPtrProxy<VT> end) {
    return { begin, end };
  }

  template <typename T>
  [[gnu::always_inline, gnu::const]]
  constexpr static PtrRange<T> New(T* begin, usize size) {
    const __ptr_range_base_t<T> begin_adaptor { begin };
    return { begin_adaptor, begin_adaptor + size };
  }

  template <typename VT>
  [[gnu::always_inline, gnu::const]]
  static PtrRange<VT> New(
   _VoidPtrProxy<VT> begin, usize size) {
    return { begin, begin + size };
  }

  template <meta::not_void T, usize N>
  [[gnu::always_inline, gnu::const]]
  constexpr static PtrRange<T> New(T(&A)[N]) {
    return { A, A + N };
  }
};

template <typename T> struct PtrRange {
  using SelfType = PtrRange<T>;
  using PtrType  = __ptr_range_base_t<T>;
  using ArrType  = std::conditional_t<meta::is_void<T>, u8, T>;
public:
  [[gnu::always_inline, gnu::const]]
  constexpr static SelfType New() {
    return { nullptr, 0 };
  }

  [[gnu::always_inline, gnu::const]]
  constexpr static SelfType New(T* begin, T* end) {
    return { PtrType(begin), PtrType(end) };
  }

  [[gnu::always_inline, gnu::const]]
  constexpr static SelfType New(T* begin, usize size) {
    const PtrType begin_adaptor { begin };
    return { begin_adaptor, begin_adaptor + size };
  }

  template <usize N>
  [[gnu::always_inline, gnu::const]]
  constexpr static SelfType New(ArrType(&A)[N]) {
    return { A, A + N };
  }

  //==================================================================//
  // Observers
  //==================================================================//

  __always_inline SelfType& checkInvariants() {
    __hc_assert(__begin <= __end);
    return *this;
  }

  const SelfType& checkInvariants() const {
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
    return ptr_distance_bytes(__begin, __end);
  }

  bool isEmpty() const {
    return size() == 0;
  }

  T* data() const __noexcept {
    return this->__begin;
  }

  template <meta::not_void U = T>
  const U& operator[](usize n) const {
    __hc_invariant(!isEmpty() && n < size());
    return __begin[n];
  }

  T* begin() const { return this->__begin; }
  T* end() const { return this->__end; }

  //==================================================================//
  // Chaining
  //==================================================================//

  [[nodiscard]] SelfType slice(usize pos, usize n) const {
    __hc_invariant(__begin && (pos + n) <= size());
    return SelfType::New(__begin + pos, n);
  }
  
  [[nodiscard]] SelfType slice(usize n) const {
    __hc_invariant(__begin && n <= size());
    return SelfType::New(__begin + n, __end);
  }

  [[nodiscard]] SelfType dropFront(usize n = 1) const {
    $tail_return slice(n);
  }

  [[nodiscard]] SelfType dropBack(usize n = 1) const {
    __hc_invariant(n <= size());
    return slice(0, size() - n);
  }

  [[nodiscard]] SelfType takeFront(usize n = 1) const {
    if (n >= size()) return *this;
    $tail_return dropBack(size() - n);
  }

  [[nodiscard]] SelfType takeBack(usize n = 1) const {
    if (n >= size()) return *this;
    $tail_return dropFront(size() - n);
  }

  __always_inline operator PtrRange<const T>() const {
    return this->intoImmRange<T>();
  }

  template <typename U>
  __always_inline PtrRange<U> intoRange() const {
    const auto new_begin = (U*)__begin;
    const auto new_end   = (U*)__end;
    return { new_begin, new_end };
  }

  template <typename U = T>
  __always_inline PtrRange<const U> intoImmRange() const {
    return this->intoRange<const U>();
  }

  template <>
  __always_inline PtrRange<void> intoRange<void>() const {
    return PtrRange<void>::New(__begin, __end);
  }

  template <>
  __always_inline PtrRange<const void>
   intoRange<const void>() const {
    return PtrRange<const void>::New(__begin, __end);
  }

  template <typename U>
  __always_inline U into() const {
    return U::New(__begin, __end);
  }

public:
  PtrType __begin = nullptr;
  PtrType __end   = nullptr;
};

template <typename T>
PtrRange(T*, T*) -> PtrRange<T>;

template <typename T>
using ImmPtrRange = PtrRange<const T>;

using AddrRange = PtrRange<void>;
using ImmAddrRange = ImmPtrRange<void>;

} // namespace hc::common
