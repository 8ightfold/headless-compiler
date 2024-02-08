//===- Common/Result.hpp --------------------------------------------===//
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

#define $Ok(args...) { ::hc::in_place,  ##args }
#define $Err(args...) { ::hc::unexpect, ##args }

namespace hc::common {
  template <typename E>
  struct Err {
    using Type = E;
  public:
    constexpr Err(const Err&) = default;
    constexpr Err(Err&&) = default;
    constexpr Err(E e) : __error(static_cast<E>(e)) { }
    constexpr E err() { return static_cast<E>(__error); }
  private:
    E __error;
  };

  template <typename E>
  Err(E&&) -> Err<E>;

  template <typename T, typename E = void>
  struct Result : __result_t<T, E> {
    using BaseType = __result_t<T, E>;
    using SelfType = Result;
    using _T = typename BaseType::Type;
    using _E = typename BaseType::Error;
  public:
    using BaseType::isOk;
    using BaseType::isErr;
  private:
    using BaseType::__pok;
    using BaseType::__perr;
    using BaseType::__destroy;

    constexpr void __initOk(auto&&...args) __noexcept {
      (void) common::construct_at(__pok(), __hc_fwd(args)...);
      BaseType::__is_value = true;
    }

    constexpr void __initErr(auto&&...args) __noexcept {
      (void) common::construct_at(__perr(), __hc_fwd(args)...);
      BaseType::__is_value = false;
    }

    constexpr void __initSelf(const Result& R) __noexcept {
      if (R.isOk())
        __initOk(R.ok());
      else
        __initErr(R.err());
    }

    constexpr void __initSelf(Result&& R) __noexcept {
      if (R.isOk())
        __initOk(__hc_move(R).ok());
      else
        __initErr(__hc_move(R).err());
    }

  public:
    /// Default construct.
    constexpr Result() = default;
    /// Copy construct.
    constexpr Result(const Result& R) {
      this->__initSelf(R);
    }
    /// Move construct.
    constexpr Result(Result&& R) {
      this->__initSelf(__hc_move(R));
    }

    template <typename G>
    constexpr Result(Err<G> e)
     : BaseType(unexpect, __hc_fwd(e.err())) { }

    template <typename U = T>
    constexpr Result(U&& u) 
     : BaseType(in_place, __hc_fwd(u)) { }
    
    /// Construct value with hint.
    constexpr Result(InPlace, auto&&...args) 
     : BaseType(in_place, __hc_fwd(args)...) { }
    
    /// Construct error with hint.
    constexpr Result(Unexpect, auto&&...args) 
     : BaseType(unexpect, __hc_fwd(args)...) { }

    constexpr ~Result() = default;

    /// Copy assign.
    constexpr Result& operator=(const Result& R) __noexcept {
      this->__destroy();
      this->__initSelf(R);
      return *this;
    }
    /// Move assign.
    constexpr Result& operator=(Result&& R) noexcept {
      this->__destroy();
      this->__initSelf(__hc_move(R));
      return *this;
    }

    template <typename G>
    constexpr Result& operator=(Err<G> e) __noexcept {
      if (isOk()) {
        this->__destroy();
        this->__initErr(__hc_fwd(e.err()));
      } else {
        this->err() = E(__hc_fwd(e.err()));
      }
      return *this;
    }
  
  public:
    constexpr static SelfType Ok(auto&&...args) {
      return SelfType(in_place, __hc_fwd(args)...);
    }

    constexpr static SelfType Err(auto&&...args) {
      return SelfType(unexpect, __hc_fwd(args)...);
    }

    //=== Accessors ===//

    constexpr _T& ok()& {
      __hc_invariant(isOk());
      return BaseType::ok(); 
    }
    constexpr const _T& ok() const& {
      __hc_invariant(isOk());
      return BaseType::ok();
    }
    constexpr _T&& ok()&& {
      __hc_invariant(isOk());
      return __hc_move(BaseType::ok());
    }

    constexpr _E& err()& {
      __hc_invariant(isErr());
      return BaseType::err();
    }
    constexpr const _E& err() const& {
      __hc_invariant(isErr());
      return BaseType::err();
    }
    constexpr _E&& err()&& {
      __hc_invariant(isErr());
      return __hc_move(BaseType::err());
    }

    constexpr _T& operator*() {
      __hc_invariant(isOk());
      return this->ok();
    }

    constexpr const _T& operator*() const {
      __hc_invariant(isOk());
      return this->ok();
    }

    constexpr _T* operator->() {
      __hc_invariant(isOk());
      return this->__pok();
    }

    constexpr const _T* operator->() const {
      __hc_invariant(isOk());
      return this->__pok();
    }

    //=== Observers ===//

    // isOk, isErr

    explicit constexpr operator bool() const __noexcept 
    { return BaseType::__is_value; }

    //=== Internals ===//

    template <typename U>
    [[gnu::always_inline, gnu::nodebug]]
    constexpr bool __isa() const __noexcept {
      if constexpr (__is_same(T, U))
        return this->isOk();
      if constexpr (__is_same(E, U))
        return this->isErr();
      return false;
    }

    template <typename U>
    [[gnu::nodebug]]
    constexpr const U* __dyn_cast() const __noexcept {
      if __expect_false(!__isa<U>())
        return nullptr;
      if constexpr (__is_same(T, E)) {
        return isOk() ? __pok() : __perr();
      } else {
        if constexpr (__is_same(T, U))
          return isOk() ? __pok() : nullptr;
        else if constexpr (__is_same(E, U))
          return isErr() ? __perr() : nullptr;
      }
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
