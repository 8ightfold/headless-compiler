//===- Common/Option.hpp --------------------------------------------===//
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

#include "_ResultBase.hpp"

#define $Some(args...) { ::hc::in_place, ##args }
#define $None() ::hc::nullopt

namespace hc::common {
  template <typename T>
  struct Option : __result_t<T, __dummy> {
    using BaseType = __result_t<T, __dummy>;
    using SelfType = Option;
  private:
    using BaseType::__pok;
    using BaseType::__perr;
    using BaseType::__destroy;

    constexpr void __initSome(auto&&...args) __noexcept {
      (void) common::construct_at(__pok(), __hc_fwd(args)...);
      BaseType::__is_value = true;
    }

    constexpr void __initNone() __noexcept {
      (void) common::construct_at(__perr());
      BaseType::__is_value = false;
    }

    constexpr void __initSelf(const Option& O) __noexcept {
      if (O.isSome())
        __initSome(O.some());
      else
        __initNone();
    }

    constexpr void __initSelf(Option&& O) __noexcept {
      if (O.isSome())
        __initSome(__hc_move(O).some());
      else
        __initNone();
    }

  public:
    /// Default construct.
    constexpr Option() = default;
    /// Copy construct.
    constexpr Option(const Option& O) {
      this->__initSelf(O);
    }
    /// Move construct.
    constexpr Option(Option&& O) {
      this->__initSelf(__hc_move(O));
    }

    constexpr Option(Nullopt) 
     : BaseType(unexpect) { }

    template <typename U = T>
    constexpr Option(U&& u) 
     : BaseType(in_place, __hc_fwd(u)) { }
    
    /// Construct value with hint.
    constexpr Option(InPlace, auto&&...args) 
     : BaseType(in_place, __hc_fwd(args)...) { }

    constexpr ~Option() = default;

    /// Copy assign.
    constexpr Option& operator=(const Option& O) __noexcept {
      this->__destroy();
      this->__initSelf(O);
      return *this;
    }
    /// Move assign.
    constexpr Option& operator=(Option&& O) noexcept {
      this->__destroy();
      this->__initSelf(__hc_move(O));
      return *this;
    }

    constexpr Option& operator=(Nullopt) __noexcept {
      this->__destroy();
      this->__initNone();
      return *this;
    }
  
  public:
    constexpr static SelfType Some(auto&&...args) {
      return SelfType(in_place, __hc_fwd(args)...);
    }

    constexpr static SelfType None() {
      return SelfType(nullopt);
    }

    //=== Accessors ===//

    constexpr T& some()& {
      __hc_invariant(isSome());
      return BaseType::ok(); 
    }
    constexpr const T& some() const& {
      __hc_invariant(isSome());
      return BaseType::ok();
    }
    constexpr T&& some()&& {
      __hc_invariant(isSome());
      return __hc_move(BaseType::ok());
    }

    constexpr T& operator*() {
      __hc_invariant(isSome());
      return this->some();
    }

    constexpr const T& operator*() const {
      __hc_invariant(isSome());
      return this->some();
    }

    constexpr T* operator->() {
      __hc_invariant(isSome());
      return this->__pok();
    }

    constexpr const T* operator->() const {
      __hc_invariant(isSome());
      return this->__pok();
    }

    //=== Observers ===//

    constexpr bool isSome() const __noexcept {
      return BaseType::__is_value;
    }

    constexpr bool isNone() const __noexcept {
      return !BaseType::__is_value;
    }

    constexpr operator bool() const __noexcept {
      return BaseType::__is_value;
    }

    //=== Monads ===//

    template <typename U>
    constexpr T valueOr(U&& u) const& {
      if(this->isSome()) 
        return some();
      else
        return T{__hc_fwd(u)};
    }

    template <typename U>
    constexpr T valueOr(U&& u)&& {
      if(this->isSome()) 
        return __hc_move(some());
      else
        return T{__hc_fwd(u)};
    }

    //=== Internals ===//

    template <typename U>
    [[gnu::always_inline, gnu::nodebug]]
    constexpr bool __isa() const __noexcept {
      if constexpr (__is_same(T, U))
        return this->isSome();
      return false;
    }

    template <typename U>
    [[gnu::nodebug]]
    constexpr const U* __dyn_cast() const __noexcept {
      if constexpr (__is_same(T, U))
        return isSome() ? __pok() : nullptr;
      return nullptr;
    }

    template <typename U>
    [[gnu::flatten, gnu::nodebug]]
    constexpr U* __dyn_cast() __noexcept {
      const SelfType* const S = this;
      return const_cast<U*>(S->__dyn_cast<U>());
    }
  };
} // namespace hc::common
