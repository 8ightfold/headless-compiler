//===- Parcel/StaticVec.hpp -----------------------------------------===//
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
//  A vector type with a statically sized array backing.
//
//===----------------------------------------------------------------===//

#pragma once
#pragma clang system_header

#include <Common/Align.hpp>
#include <Common/Lifetime.hpp>
#include <Common/PtrRange.hpp>
#include <Common/Option.hpp>
#include <Meta/Traits.hpp>
#include "Common.hpp"

/// Creates a `StaticVec` deduced from the passed arguments.
/// Has a capacity rounded to the next power of 2 (inclusively).
#define $Vec(args...) ::hc::parcel::__make_staticvec(args)

namespace hc::parcel {
  struct _StaticVecVars { };

  //====================================================================//
  // Base
  //====================================================================//

  template <typename T>
  struct StaticVecPtrBase {
    constexpr T* data() __noexcept {
      return this->__first_elem;
    }
    constexpr const T* data() const __noexcept {
      return this->__first_elem;
    }
  protected:
    constexpr void __setPtr(T* P) {
      this->__first_elem = P;
    }
  public:
    T* __first_elem;
  };

  template <typename SizeType>
  struct StaticVecCapBase {
    using Type = SizeType;
  public:
    constexpr static SizeType SizeTypeMax() noexcept {
      return Max<SizeType>;
    }

    constexpr SizeType size() const __noexcept {
      return this->__size;
    }
    constexpr SizeType capacity() const __noexcept {
      return this->__cap;
    }

    [[nodiscard]] constexpr bool isEmpty() const __noexcept {
      return (__size == 0);
    }
    [[nodiscard]] constexpr bool isFull() const __noexcept {
      __hc_assert(__size < __cap);
      return (__size == __cap);
    }
  
  protected:
    void __growBy(usize N = 1) {
      const auto NN = static_cast<SizeType>(N);
      __hc_assert((__size + NN) <= this->__cap);
      this->__size += NN;
    }

    void __shrinkBy(usize N = 1) {
      const usize NN = (__size >= N) ? N : __size;
      this->__size -= static_cast<SizeType>(NN);
    }

    void __setSize(usize N) {
      const auto NN = static_cast<SizeType>(N);
      __hc_assert(NN <= this->__cap);
      this->__size = NN;
    }

  public:
    SizeType __size, __cap;
  };

  template <typename T>
  using StaticVecCapType = meta::__conditional_t<
    (sizeof(T) < sizeof(uhalfptr)), uptr, uhalfptr>;
  
  template <typename T>
  using StaticVecSzCapBase =
    StaticVecCapBase<StaticVecCapType<T>>;

  template <typename T, typename = void>
  struct StaticVecBase : 
   public StaticVecPtrBase<T>, 
   public StaticVecSzCapBase<T> {
    using PtrBase = StaticVecPtrBase<T>;
    using CapBase = StaticVecSzCapBase<T>;
    using CapType = typename CapBase::Type;
  public:
    using PtrBase::data;
    using CapBase::size;
    using CapBase::capacity;
    using CapBase::isEmpty;
    using CapBase::isFull;
  protected:
    using PtrBase::__setPtr;
    using CapBase::__growBy;
    using CapBase::__shrinkBy;
    using CapBase::__setSize;
  private:
    using CapBase::SizeTypeMax;
    using PtrBase::__first_elem;
    using CapBase::__size;
    using CapBase::__cap;
  
  public:
    StaticVecBase() = delete;
    constexpr StaticVecBase(T* P, usize cap) : 
     StaticVecPtrBase<T>(P),
     StaticVecSzCapBase<T>(0, CapType(cap)) {}

  public:
    static constexpr usize MaxSize() {
      constexpr usize L = Max<usize> / sizeof(T);
      constexpr usize R = SizeTypeMax();
      return (L < R) ? L : R;
    }

    constexpr CapType sizeInBytes() const __noexcept {
      return __size * __sizeof(T);
    }
    constexpr CapType capacityInBytes() const __noexcept {
      return __cap * __sizeof(T);
    }

    constexpr CapType __remainingCapacity() const __noexcept {
      return (__cap - __size);
    }

    constexpr T* begin() { return data(); }
    constexpr T* end() { return begin() + size(); }
    constexpr const T* begin() const { return data(); }
    constexpr const T* end() const { return begin() + size(); }

    [[nodiscard]] constexpr T& 
     operator[](usize N) __noexcept {
      __hc_assert(N < __size);
      return data()[N];
    }
    [[nodiscard]] constexpr const T& 
     operator[](usize N) const __noexcept {
      __hc_assert(N < __size);
      return data()[N];
    }

    constexpr T& front() {
      __hc_assert(!isEmpty());
      return begin()[0];
    }
    constexpr const T& front() const {
      __hc_assert(!isEmpty());
      return begin()[0];
    }

    constexpr T& back() {
      __hc_assert(!isEmpty());
      return end()[-1];
    }
    constexpr const T& back() const {
      __hc_assert(!isEmpty());
      return end()[-1];
    }
  };

  //====================================================================//
  // Storage
  //====================================================================//

  template <typename T, usize BufferSize>
  union alignas(T) _StaticVecNPODStorage {
    static constexpr usize __uSize = __sizeof(T) * BufferSize;
  public:
    constexpr _StaticVecNPODStorage() : __pad() {}
    constexpr ~_StaticVecNPODStorage() { }

    __always_inline constexpr
    T* __data() __noexcept 
    { return this->__buffer; }

    __always_inline constexpr
    const T* __data() const __noexcept 
    { return this->__buffer; }

  public:
    ubyte __pad[__uSize];
    T __buffer[BufferSize];
  };

  template <typename T, usize BufferSize>
  struct alignas(T) StaticVecStorage {
    using Type = _StaticVecNPODStorage<T, BufferSize>;
  public:
    inline constexpr
    T* __data() __noexcept 
    { return __sto.__buffer; }

    inline constexpr
    const T* __data() const __noexcept 
    { return __sto.__buffer; }

  public:
    mutable Type __sto;
  };

  template <typename T, usize BufferSize>
  requires meta::is_trivially_relocatable<T>
  struct alignas(T) StaticVecStorage<T, BufferSize> {
    using Type = T[BufferSize];
  public:
    inline constexpr
    T* __data() __noexcept 
    { return this->__sto; }

    inline constexpr
    const T* __data() const __noexcept 
    { return this->__sto; }

  public:
    mutable Type __sto;
  };

  template <typename T>
  struct alignas(T) StaticVecStorage<T, 0U> {
    using Type = void;
  public:
    inline constexpr
    T* __data() __noexcept 
    { return nullptr; }

    inline constexpr
    const T* __data() const __noexcept 
    { return nullptr; }
  };

  //====================================================================//
  // Implementation
  //====================================================================//

  template <typename T,
    bool = meta::is_trivially_relocatable<T>>
  struct IStaticVecBase : public StaticVecBase<T> {
  protected:
    static constexpr bool passByValue = false;
    using PassType = const T&;

    constexpr IStaticVecBase(T* P, usize cap) : 
     StaticVecBase<T>(P, cap) {}

    static constexpr void DestroyRange(T* I, T* E) {
      // Run dtors in reverse order.
      while (I != E) {
        (--E)->~T();
      }
    }

    constexpr T* growUninit(usize N = 1) {
      __hc_invariant(N != 0);
      const auto old_end = this->end();
      this->__growBy(N);
      return old_end;
    }

    void growAndAssign(usize N, const T& V) {
      T* I = this->growUninit(N);
      while (I != this->end()) {
        com::construct_at(I, V);
        ++I;
      }
    }

  public:
    constexpr void push(const T& V) {
      if __expect_false(this->isFull())
        return;
      T* P = this->growUninit();
      (void) com::construct_at(P, V);
    }

    constexpr void push(T&& V) {
      if __expect_false(this->isFull())
        return;
      T* P = this->growUninit();
      (void) com::construct_at(P, __hc_move(V));
    }

    constexpr void pop() {
      if __expect_false(this->isEmpty())
        return;
      this->__shrinkBy(1);
      this->end()->~T();
    }

    constexpr com::Option<T&> pushBack(const T& V) {
      if __expect_false(this->isFull())
        return $None();
      this->push(V);
      return $Some(this->back());
    }

    constexpr com::Option<T&> pushBack(T&& V) {
      if __expect_false(this->isFull())
        return $None();
      this->push(__hc_move(V));
      return $Some(this->back());
    }

    constexpr com::Option<T> popBack() {
      if __expect_false(this->isEmpty())
        return $None();
      T bk = __hc_move(this->back());
      this->pop();
      return $Some(__hc_move(bk));
    }
  };

  template <typename T>
  struct IStaticVecBase<T, true> : public StaticVecBase<T> {
  protected:
    static constexpr bool passByValue = sizeof(T) < (2 * sizeof(void*));
    using PassType = meta::__conditional_t<passByValue, T, const T&>;

    constexpr IStaticVecBase(T* P, usize cap) : 
     StaticVecBase<T>(P, cap) {}
    
    /// Do nothing with trivial types.
    static constexpr void DestroyRange(T*, T*) {}

    constexpr T* growUninit(usize N = 1) {
      __hc_invariant(N != 0);
      const auto old_end = this->end();
      this->__growBy(N);
      return old_end;
    }

    void growAndAssign(usize N, const T& V) {
      T* I = this->growUninit(N);
      while (I != this->end()) {
        com::construct_at(I, V);
        ++I;
      }
    }
  
  public:
    constexpr void push(PassType V) {
      if __expect_false(this->isFull())
        return;
      T* P = this->growUninit();
      if $is_consteval() {
        com::construct_at(P, V);
      } else {
        (void) com::Mem::VCopy(P, &V, sizeof(T));
      }
    }

    constexpr void pop() {
      if __expect_false(this->isEmpty())
        return;
      this->__shrinkBy(1);
    }

    constexpr com::Option<T&> pushBack(PassType V) {
      if __expect_false(this->isFull())
        return $None();
      this->push(V);
      return $Some(this->back());
    }

    constexpr com::Option<T> popBack() {
      if __expect_false(this->isEmpty())
        return $None();
      T bk = this->back();
      this->pop();
      return $Some(bk);
    }
  };

  /// @brief The base for "dynamic" arrays.
  /// Assumes there is a predefined maximum capacity.
  /// Use this when passing vectors as function arguments
  /// to avoid unnecessary instantiations.
  /// @tparam T The array element type.
  template <typename T>
  struct [[gsl::Pointer]] IStaticVec : public IStaticVecBase<T> {
    using BaseType = IStaticVecBase<T>;
    using SelfType = IStaticVec<T>;
    using Type = T;
  protected:
    constexpr IStaticVec(T* P, usize cap) :
     IStaticVecBase<T>(P, cap) {}

  public:
    using BaseType::begin;
    using BaseType::end;
    using BaseType::growUninit;

    constexpr bool resizeUninit(usize n) {
      if __expect_true(n < this->capacity()) {
        this->__setSize(n);
        return true;
      }
      return false;
    }

    constexpr void emplace(auto&&...args) {
      if __expect_false(this->isFull())
        return;
      T* P = BaseType::growUninit();
      com::construct_at(P, __hc_fwd(args)...);
    }

    constexpr com::Option<T&> emplaceBack(auto&&...args) {
      if __expect_false(this->isFull())
        return $None();
      this->emplace(__hc_fwd(args)...);
      return $Some(this->back());
    }

    [[nodiscard]]
    com::PtrRange<T> intoRange() __noexcept {
      return { begin(), end() };
    }

    [[nodiscard]]
    com::ImmPtrRange<T> intoRange() const __noexcept {
      return { begin(), end() };
    }

    template <typename U>
    [[nodiscard]]
    inline U into() __noexcept {
      return intoRange().template into<U>();
    }

    template <typename U>
    [[nodiscard]]
    inline U into() const __noexcept {
      return intoRange().template into<U>();
    }

    constexpr void clear() __noexcept {
      return this->__destroy();
    }
  
  protected:
    constexpr void __init(usize N, const T& D = T()) {
      this->growAndAssign(N, D);
    }

    constexpr void __initFromPack(auto&&...args) {
      (this->emplace(__hc_fwd(args)), ...);
    }

    constexpr void __destroy() {
      BaseType::DestroyRange(
        this->begin(), this->end());
    }
  };

  /// @brief Dynamic array with a static backing.
  /// @tparam T The array element type.
  /// @tparam BufferSize The maximum amount of elements.
  template <typename T, usize BufferSize>
  struct [[gsl::Owner]] StaticVec :
   public IStaticVec<T>,
   protected StaticVecStorage<T, BufferSize> {
    static_assert(!__is_array(T));
    static_assert(__is_object(T));
    using BaseType  = IStaticVec<T>;
    using StorageType = StaticVecStorage<T, BufferSize>;
    using SelfType  = StaticVec;
    using Type = T;
    static constexpr usize __capacity = BufferSize;
  public:
    constexpr StaticVec() : 
     BaseType(this->__data(), __capacity), StorageType() {}

    template <typename...Args>
    requires(sizeof...(Args) <= __capacity)
    constexpr StaticVec(_StaticVecVars, Args&&...args) : StaticVec() {
      this->__initFromPack(__hc_fwd(args)...);
    }

    template <typename...Args>
    requires((sizeof...(Args) + 1) <= __capacity)
    constexpr StaticVec(const T& t, Args&&...args) : StaticVec() {
      this->__initFromPack(t, __hc_fwd(args)...);
    }

    constexpr StaticVec(usize n, const T& D = T()) : StaticVec() {
      __hc_assert(n <= __capacity);
      this->__init(n, D);
    }

    StaticVec(com::PtrRange<T> R) : StaticVec() {
      if __expect_false(R.isEmpty()) return;
      const auto n = __Cap(R.size());
      com::Mem::Copy(this->growUninit(n), R.begin(), n);
    }

    template <usize Sz>
    StaticVec(const StaticVec<T, Sz>& V) : StaticVec() {
      if constexpr (Sz > Capacity()) {
        const auto n = __Cap(V.size());
        com::Mem::Copy(this->growUninit(n), V.begin(), n);
      } else {
        const auto n = V.size();
        com::Mem::Copy(this->growUninit(n), V.begin(), n);
      }
    }

    template <usize Sz>
    StaticVec(StaticVec<T, Sz>&& V) : StaticVec() {
      if constexpr (Sz > Capacity()) {
        const auto n = __Cap(V.size());
        com::Mem::Move(this->growUninit(n), V.begin(), n);
      } else {
        const auto n = V.size();
        com::Mem::Move(this->growUninit(n), V.begin(), n);
      }
    }

    constexpr ~StaticVec() { this->__destroy(); }

    //==================================================================//
    // Mutators
    //==================================================================//

    constexpr SelfType& erase() __noexcept {
      this->clear();
      return *this;
    }

    //==================================================================//
    // Observers
    //==================================================================//

    static constexpr usize Capacity() __noexcept {
      return SelfType::__capacity;
    }

    constexpr usize remainingCapacity() const __noexcept {
      return Capacity() - this->size();
    }
  
  private:
    [[gnu::always_inline, gnu::const]]
    inline constexpr static usize __Cap(usize n) {
      return (n > Capacity()) ? Capacity() : n;
    }
  };

  template <typename T, usize N>
  StaticVec(const StaticVec<T, N>&) -> StaticVec<T, N>;

  template <typename T, usize N>
  StaticVec(StaticVec<T, N>&&) -> StaticVec<T, N>;

  //====================================================================//
  // Deduction
  //====================================================================//

  /// @brief `StaticVec` with the element count rounded to the next power of 2.
  /// @tparam T The array element type.
  /// @tparam N The minimum amount of elements
  template <typename T, usize N>
  using ALStaticVec = StaticVec<T, com::Align::Up(N)>;

  template <typename T, typename...TT>
  using __staticvec_t = StaticVec<__decay(T), 
    com::Align::Up(sizeof...(TT) + 1)>;
  
  template <typename T, typename...TT>
  StaticVec(T&&, TT&&...) -> StaticVec<__decay(T), 
    com::Align::Up(sizeof...(TT) + 1)>;

  template <typename T, typename...TT>
  [[nodiscard, gnu::always_inline, gnu::nodebug]]
  __visibility(hidden) inline constexpr auto
   __make_staticvec(T&& t, TT&&...tt) __noexcept {
    using  VecType = __staticvec_t<T, TT...>;
    return VecType { _StaticVecVars{}, __hc_fwd(t), __hc_fwd(tt)... };
  }
} // namespace hc::parcel
