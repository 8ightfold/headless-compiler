//===- Common/RawLazy.hpp -------------------------------------------===//
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
//  Defines a manually constructed/destructed wrapper around a type.
//
//===----------------------------------------------------------------===//

#pragma once

#include "Fundamental.hpp"
#include "Lifetime.hpp"

namespace hc::common {
  template <typename T>
  union alignas(T) [[clang::trivial_abi]] _RawLazyBase {
    using Type = T;
    using SelfType = _RawLazyBase<T>;
    using BufType = ubyte[sizeof(T)];
  public:
    constexpr ~_RawLazyBase() { }
  public:
    alignas(T) BufType __buf;
    T __data;
  };

  template <typename T>
  struct RawLazy {
    using Type = T;
    using SelfType = RawLazy<T>;
    using BaseType = _RawLazyBase<T>;
    using BufType  = typename BaseType::BufType;
  public:
    constexpr ~RawLazy() = default;

    __always_inline constexpr 
     T& ctor(auto&&...args) __noexcept {
      T* P = common::construct_at(
        &unwrap(), __hc_fwd(args)...);
      return *P;
    }

    __always_inline constexpr
     void dtor() noexcept {
      destroy_at_ref(this->unwrap());
    }

    __ndbg_inline constexpr
     T& unwrap() noexcept {
      return __data.__data;
    }

    __ndbg_inline constexpr
     const T& unwrap() const noexcept {
      return __data.__data;
    }

    __ndbg_inline constexpr
     T* data() noexcept {
      return &__data.__data;
    }

    __ndbg_inline constexpr
     const T* data() const noexcept {
      return &__data.__data;
    }

    __ndbg_inline constexpr T take() __noexcept {
      T V {__hc_move(this->unwrap())};
      this->dtor();
      return V;
    }

    //=== Internals ===//

    __ndbg_inline constexpr
     BufType& __buf() noexcept {
      return __data.__buf;
    }

    __ndbg_inline constexpr
     const BufType& __buf() const noexcept {
      return __data.__buf;
    }

  public:
    BaseType __data;
  };

  template <typename T>
  requires(meta::is_trivial<T>)
  struct [[clang::trivial_abi]] RawLazy<T> {
    using Type = T;
    using SelfType = RawLazy<T>;
    using BaseType = T;
    using BufType  = void;
  public:
    __always_inline constexpr 
     T& ctor(auto&&...args) noexcept {
      return (unwrap() = T{__hc_fwd(args)...});
    }

    __always_inline constexpr
     void dtor() const noexcept { }

    __ndbg_inline constexpr
     T& unwrap() noexcept {
      return __data;
    }

    __ndbg_inline constexpr
     const T& unwrap() const noexcept {
      return __data;
    }

    __ndbg_inline constexpr T take() noexcept {
      return __data;
    }

  public:
    BaseType __data;
  };

  static_assert(meta::is_trivial<RawLazy<int>>);
} // namespace hc::common
