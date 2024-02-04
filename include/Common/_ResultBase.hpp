//===- Common/_ResultBase.hpp ---------------------------------------===//
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

#include "Fundamental.hpp"
#include "Lifetime.hpp"

//=== Globals ===//
namespace hc {
  struct Nullopt { };
  struct Unexpect { };
  struct InPlace { };

  __global Nullopt nullopt { };
  __global Unexpect unexpect { };
  __global InPlace in_place { };
} // namespace hc

//=== Backend ===//
namespace hc::common {
  template <typename T>
  struct _ResultVoid {
    using Type = T;
  };

  template <>
  struct _ResultVoid<void> {
    using Type = __void;
  };

  template <typename T>
  using __result_void_t = typename
    _ResultVoid<__remove_const(T)>::Type;

  template <typename T, typename E>
  union _ResultBase {
    constexpr _ResultBase() : __value() { }

    constexpr _ResultBase(InPlace, auto&&...args)
     : __value(__hc_fwd(args)...) { }
    
    constexpr _ResultBase(Unexpect, auto&&...args)
     : __error(__hc_fwd(args)...) { }
    
    constexpr ~_ResultBase() { }
  public:
    T __value;
    E __error;
  };

  template <typename T, typename E>
  struct _ResultStorage {
    using Type = T;
    using Error = E;
    using _BaseType = _ResultBase<T, E>;
  public:
    constexpr _ResultStorage() = default;

    constexpr _ResultStorage(InPlace ip, auto&&...args)
     : __base(ip, __hc_fwd(args)...), __is_value(true) { }
    
    constexpr _ResultStorage(Unexpect ux, auto&&...args)
     : __base(ux, __hc_fwd(args)...), __is_value(false) { }
    
    constexpr ~_ResultStorage() {
      this->__destroy();
    }

  public:
    constexpr bool isOk() const noexcept {
      return this->__is_value;
    }

    constexpr bool isErr() const noexcept {
      return !this->__is_value;
    }

    __always_inline constexpr T& ok()& { return __base.__value; }
    __always_inline constexpr const T& ok() const& { return __base.__value; }
    __always_inline constexpr T&& ok()&& { return static_cast<T&&>(__base.__value); }

    __always_inline constexpr E& err()& { return __base.__error; }
    __always_inline constexpr const E& err() const& { return __base.__error; }
    __always_inline constexpr E&& err()&& { return static_cast<E&&>(__base.__error); }

  protected:
    constexpr void __destroy() noexcept {
      if (this->__is_value)
        common::destroy_at(__pok());
      else
        common::destroy_at(__perr());
    }

    __always_inline constexpr T* __pok() noexcept {
      return common::__addressof(__base.__value);
    }

    __always_inline constexpr const T* __pok() const noexcept {
      return common::__addressof(__base.__value);
    }

    __always_inline constexpr E* __perr() noexcept {
      return common::__addressof(__base.__error);
    }

    __always_inline constexpr const E* __perr() const noexcept {
      return common::__addressof(__base.__error);
    }

  protected:
    _BaseType __base;
    bool __is_value = true;
  };

  template <typename T, typename E>
  using __result_t = _ResultStorage<
    __result_void_t<T>, __result_void_t<E>>;
} // namespace hc::common
