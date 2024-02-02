//===- Common/StaticVec.hpp -----------------------------------------===//
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

#include "Fundamental.hpp"
#include "Lifetime.hpp"
#include "PtrRange.hpp"
#include "Traits.hpp"

#define $vec(args...) ::hc::common::__make_staticvec(args)

namespace hc::common {
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
    u8 __pad[__uSize];
    T __buffer[BufferSize];
  };

  template <typename T, usize BufferSize>
  struct StaticVec {
    static_assert(!__is_array(T));
    static_assert(__is_object(T));
    using _BaseType = _StaticVecBase<T, BufferSize>;
    using SelfType  = StaticVec;
    using Type = T;
    static constexpr usize capacity = BufferSize;
  public:
    StaticVec() = default;

    template <typename...Args>
    requires(sizeof...(Args) <= capacity)
    constexpr StaticVec(_StaticVecVars, Args&&...args) 
     : __base(), __size(sizeof...(Args)) {
      using Seq = make_idxseq<sizeof...(Args)>;
      this->__init_from_pack(Seq{}, __hc_fwd(args)...);
    }

    constexpr StaticVec(usize n, const T& D) 
     : __base(), __size(n) {
      __hc_assert(n <= capacity);
      this->__init(begin(), end(), D);
    }

    //=== Mutators/Accessors ===//

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

    // TODO: Return optional

    constexpr SelfType& push(const T& t) __noexcept {
      (void)__init_back(t);
      return *this;
    }

    constexpr SelfType& emplace(auto&&...args) __noexcept {
      (void)__init_back(__hc_fwd(args)...);
      return *this;
    }

    constexpr SelfType& pop() __noexcept {
      (void)__destroy_back();
      return *this;
    }

    constexpr T& front() const __noexcept {
      __hc_invariant(size() > 0);
      return data()[0];
    }

    constexpr T& back() const __noexcept {
      __hc_invariant(size() > 0);
      return data()[__size - 1];
    }

    //=== Observers ===//

    static constexpr usize Capacity() __noexcept {
      return SelfType::capacity;
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
    PtrRange<T> toPtrRange() const __noexcept {
      return { .__begin = begin(), .__end = end() };
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
  
  private:
    struct _DestroyVec {
      constexpr _DestroyVec(StaticVec& vec) : __vec(vec) { }
      
      constexpr void operator()() __noexcept {
        if __expect_true(__vec.__size > 0)
          __destroy(__vec.begin(), __vec.end());
      }

    private:
      StaticVec& __vec;
    };
  
  public:
    constexpr ~StaticVec() { _DestroyVec{*this}(); }

  private:
    template <usize...II>
    __always_inline constexpr void __init_from_pack(IdxSeq<II...>, auto&&...args) {
      static_assert(sizeof...(II) == sizeof...(args));
      const auto D = data();
      (((void)common::construct_at(D + II, __hc_fwd(args))), ...);
    }

    __always_inline constexpr void __init(T* I, T* E, const T& D) {
      for(; I != E; ++I)
        (void)common::construct_at(I, D);
    }

    __always_inline constexpr bool __init_back(auto&&...args) {
      if __expect_false(size() == Capacity())
        return false;
      (void)common::construct_at(end(), __hc_fwd(args)...);
      ++__size;
      return true;
    }

    __always_inline constexpr bool __destroy_back() {
      if __expect_false(size() == 0)
        return false;
      common::destroy_at(end() - 1);
      --__size;
      return true;
    }

  private:
    mutable _BaseType __base { };
    usize __size = 0;
  };

  template <typename T>
  struct StaticVec<T, 0> {
    static_assert(!__is_array(T));
    using SelfType  = StaticVec;
    using Type = T;
    static constexpr usize capacity = 0;
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
    PtrRange<T> toPtrRange() const __noexcept { return {}; }

    [[nodiscard, gnu::const]]
    constexpr T* begin() const __noexcept { return nullptr; }

    [[nodiscard, gnu::const]]
    constexpr T* end() const __noexcept { return nullptr; }

    [[nodiscard, gnu::const]]
    constexpr bool isEmpty() const __noexcept { return true; }
  };

  template <typename T, typename...TT>
  [[nodiscard, gnu::always_inline, gnu::nodebug]]
  __visibility(hidden) inline constexpr auto
   __make_staticvec(T&& t, TT&&...tt) __noexcept {
    using VecType = StaticVec<__remove_reference_t(T), sizeof...(TT) + 1>;
    return VecType { _StaticVecVars{}, __hc_fwd(t), __hc_fwd(tt)... };
  }
} // namespace hc::common
