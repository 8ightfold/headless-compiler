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

  template <typename T, usize BufferSize>
  union alignas(T) _StaticVecBase {
    static constexpr usize __uSize = __sizeof(T) * BufferSize;
  public:
    constexpr ~_StaticVecBase() { }

    __always_inline constexpr T* 
     __data() __noexcept 
    { return this->__buffer; }

    __always_inline constexpr const T* 
     __data() const __noexcept 
    { return this->__buffer; }

  public:
    ubyte __pad[__uSize];
    T __buffer[BufferSize];
  };

  template <typename T, usize BufferSize>
  struct StaticVec {
    static_assert(!__is_array(T));
    static_assert(__is_object(T));
    using _BaseType = _StaticVecBase<T, BufferSize>;
    using SelfType  = StaticVec;
    using Type = T;
    static constexpr usize __capacity = BufferSize;
  public:
    StaticVec() = default;

    template <typename...Args>
    requires(sizeof...(Args) <= __capacity)
    constexpr StaticVec(_StaticVecVars, Args&&...args) 
     : __base(), __size(sizeof...(Args)) {
      using Seq = CC::make_idxseq<sizeof...(Args)>;
      this->__initFromPack(Seq{}, __hc_fwd(args)...);
    }

    template <typename...Args>
    requires((sizeof...(Args) + 1) <= __capacity)
    constexpr StaticVec(const T& t, Args&&...args) 
     : __base(), __size(sizeof...(Args) + 1) {
      using Seq = CC::make_idxseq<sizeof...(Args) + 1>;
      this->__initFromPack(Seq{}, t, __hc_fwd(args)...);
    }

    constexpr StaticVec(usize n, const T& D = T()) 
     : __base(), __size(n) {
      __hc_assert(n <= __capacity);
      this->__init(begin(), end(), D);
    }

    StaticVec(CC::PtrRange<T> R) : StaticVec() {
      if __expect_false(R.isEmpty()) return;
      const auto n = __Cap(R.size());
      CC::Mem::Copy(begin(), R.begin(), n);
      this->__size = n;
    }

    template <usize Sz>
    StaticVec(const StaticVec<T, Sz>& V) : StaticVec() {
      if constexpr (Sz > Capacity()) {
        const auto n = __Cap(V.size());
        CC::Mem::Copy(begin(), V.begin(), n);
        this->__size = n;
      } else {
        CC::Mem::Copy(begin(), V.begin(), V.__size);
        this->__size = V.__size;
      }
    }

    template <usize Sz>
    StaticVec(StaticVec<T, Sz>&& V) : StaticVec() {
      if constexpr (Sz > Capacity()) {
        const auto n = (V.__size > Capacity()) 
          ? Capacity() : V.__size;
        CC::Mem::Move(begin(), V.begin(), n);
        this->__size = n;
      } else {
        CC::Mem::Move(begin(), V.begin(), V.__size);
        this->__size = V.__size;
      }
    }

    constexpr ~StaticVec() { __destroy(); }

    //==================================================================//
    // Mutators/Accessors
    //==================================================================//

    [[nodiscard]] constexpr T& 
     operator[](usize n) __noexcept {
      __hc_invariant(n < __size);
      return this->data()[n];
    }

    [[nodiscard]] constexpr const T& 
     operator[](usize n) const __noexcept {
      __hc_invariant(n < __size);
      return this->data()[n];
    }

    constexpr SelfType& push(const T& t) __noexcept {
      (void) __initBack(t);
      return *this;
    }

    constexpr SelfType& emplace(auto&&...args) __noexcept {
      (void) __initBack(__hc_fwd(args)...);
      return *this;
    }

    constexpr SelfType& pop() __noexcept {
      (void) __destroyBack();
      return *this;
    }

    constexpr CC::Option<T> popBack() __noexcept {
      if (this->isEmpty())
        return $None();
      CC::Option<T> O = $Some(back()); 
      (void) __destroyBack();
      return O;
    }

    constexpr T& front() const __noexcept {
      __hc_invariant(size() > 0);
      return data()[0];
    }

    constexpr T& back() const __noexcept {
      __hc_invariant(size() > 0);
      return data()[__size - 1];
    }

    constexpr bool resize(usize N) __noexcept {
      T* const old_end = end();
      const bool R = resizeUninit();
      this->__init(old_end, end(), T());
      return R;
    }

    /// Can be very unsafe, do not use without good reason.
    constexpr bool resizeUninit(usize N) __noexcept {
      const usize new_size = N + size();
      if __expect_false(new_size > Capacity()) {
        __size = Capacity();
        return false;
      }
      __size = new_size;
      return true;
    }

    constexpr SelfType& erase() __noexcept {
      __destroy();
      return *this;
    }

    constexpr void clear() __noexcept {
      $tail_return __destroy();
    }

    //==================================================================//
    // Observers
    //==================================================================//

    static constexpr usize Capacity() __noexcept {
      return SelfType::__capacity;
    }

    constexpr usize remainingCapacity() const __noexcept {
      return Capacity() - size();
    }

    [[nodiscard, gnu::const]]
    constexpr T* data() const __noexcept {
      return this->__base.__data();
    }

    constexpr usize size() const __noexcept {
      return this->__size;
    }

    constexpr usize sizeInBytes() const __noexcept {
      return this->__size * __sizeof(T);
    }

    [[nodiscard]]
    CC::PtrRange<T> intoRange() __noexcept {
      return { begin(), end() };
    }

    [[nodiscard]]
    CC::ImmPtrRange<T> intoRange() const __noexcept {
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

    [[nodiscard, gnu::const]]
    constexpr T* begin() const __noexcept {
      return data();
    }

    [[nodiscard]]
    constexpr T* end() const __noexcept {
      return data() + size();
    }

    [[nodiscard]]
    constexpr bool isEmpty() const __noexcept {
      return this->__size == 0;
    }

    //==================================================================//
    // Internals
    //==================================================================//

    friend constexpr usize& __get_size_ref(StaticVec& V) {
      return V.__size;
    }
  
  private:
    struct _DestroyVec {
      constexpr _DestroyVec(StaticVec& V) : __vec(V) { }
      
      constexpr void operator()() __noexcept {
        if __expect_true(__vec.__size > 0)
          CC::__destroy(__vec.begin(), __vec.end());
      }

    private:
      StaticVec& __vec;
    };
  
    template <usize...II>
    __always_inline constexpr void __initFromPack(
     CC::IdxSeq<II...>, auto&&...args) {
      static_assert(sizeof...(II) == sizeof...(args));
      const auto D = data();
      (((void) CC::construct_at(D + II, __hc_fwd(args))), ...);
    }

    __always_inline constexpr void __init(T* I, T* E, const T& D) {
      for (; I != E; ++I)
        (void) CC::construct_at(I, D);
    }

    __always_inline constexpr bool __initBack(auto&&...args) {
      if __expect_false(size() == Capacity())
        return false;
      (void) CC::construct_at(end(), __hc_fwd(args)...);
      ++__size;
      return true;
    }

    __always_inline constexpr void __destroy() {
      _DestroyVec{*this}();
      this->__size = 0;
    }

    __always_inline constexpr bool __destroyBack() {
      if __expect_false(size() == 0)
        return false;
      CC::destroy_at(end() - 1);
      --__size;
      return true;
    }

    [[gnu::always_inline, gnu::const]]
    inline constexpr static usize __Cap(usize n) {
      return (n > Capacity()) ? Capacity() : n;
    }

  public:
    mutable _BaseType __base { };
    usize __size = 0;
  };

  template <typename T>
  struct StaticVec<T, 0> {
    static_assert(!__is_array(T));
    using SelfType  = StaticVec;
    using Type = T;
    static constexpr usize __capacity = 0;
  public:
    [[nodiscard, gnu::const]]
    static constexpr usize Capacity() __noexcept { return 0; }

    [[nodiscard, gnu::const]]
    constexpr T* data() const __noexcept { return nullptr; }

    [[nodiscard, gnu::const]]
    constexpr usize size() const __noexcept { return 0; }

    [[nodiscard, gnu::const]]
    constexpr usize sizeInBytes() const __noexcept { return 0; }

    [[nodiscard, gnu::const]]
    CC::PtrRange<T> intoRange() __noexcept { return {}; }

    [[nodiscard, gnu::const]]
    CC::ImmPtrRange<T> intoRange() const __noexcept { return {}; }

    [[nodiscard, gnu::const]]
    constexpr T* begin() const __noexcept { return nullptr; }

    [[nodiscard, gnu::const]]
    constexpr T* end() const __noexcept { return nullptr; }

    [[nodiscard, gnu::const]]
    constexpr bool isEmpty() const __noexcept { return true; }
  };

  template <typename T, usize N>
  StaticVec(const StaticVec<T, N>&) -> StaticVec<T, N>;

  template <typename T, usize N>
  StaticVec(StaticVec<T, N>&&) -> StaticVec<T, N>;

  //====================================================================//
  // Deduction
  //====================================================================//

  template <typename T, usize N>
  using ALStaticVec = StaticVec<T, CC::Align::Up(N)>;

  template <typename T, typename...TT>
  using __staticvec_t = StaticVec<__decay(T), 
    CC::Align::Up(sizeof...(TT) + 1)>;
  
  template <typename T, typename...TT>
  StaticVec(T&&, TT&&...) -> StaticVec<__decay(T), 
    CC::Align::Up(sizeof...(TT) + 1)>;

  template <typename T, typename...TT>
  [[nodiscard, gnu::always_inline, gnu::nodebug]]
  __visibility(hidden) inline constexpr auto
   __make_staticvec(T&& t, TT&&...tt) __noexcept {
    using  VecType = __staticvec_t<T, TT...>;
    return VecType { _StaticVecVars{}, __hc_fwd(t), __hc_fwd(tt)... };
  }
} // namespace hc::parcel
