//===- Common/DefaultFuncPtr.hpp ------------------------------------===//
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
//  This file defines a temporary function for safe stubs.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Meta/Traits.hpp>

namespace hc::common {
  template <typename Ret, typename...Args>
  requires meta::is_trivially_default_constructible<Ret>
  inline Ret default_func(Args...) { return Ret{}; }

  template <typename Ret, typename...Args>
  requires meta::is_void<Ret>
  inline void default_func(Args...) { }

  template <typename Ret, typename...Args>
  requires meta::is_trivially_default_constructible<Ret>
  inline Ret default_vfunc(Args..., ...) { return Ret{}; }

  template <typename Ret, typename...Args>
  requires meta::is_void<Ret>
  inline void default_vfunc(Args..., ...) { }

  template <typename Ret, typename...Args>
  __global Ret(*default_func_ptr)(Args...) = &default_func<Ret, Args...>;

  template <typename Ret, typename...Args>
  __global Ret(*default_vfunc_ptr)(Args..., ...) = &default_vfunc<Ret, Args...>;

  ////////////////////////////////////////////////////////////////////////

  template <typename T>
  struct _DefaultFuncPtrBase;

  template <typename Ret, typename...Args>
  struct _DefaultFuncPtrBase<Ret(Args...)> {
    using CallbackType = Ret(Args...);
    static constexpr CallbackType* defaultCB
      = default_func_ptr<Ret, Args...>;
  public:
    __always_inline Ret operator()(Args...args) const {
      return __cb(__hc_fwd_<Args>(args)...);
    }
  public:
    CallbackType* __cb = defaultCB;
  };

  template <typename Ret, typename...Args>
  struct _DefaultFuncPtrBase<Ret(Args..., ...)> {
    using CallbackType = Ret(Args..., ...);
    static constexpr CallbackType* defaultCB
      = default_vfunc_ptr<Ret, Args...>;
  public:
    __always_inline Ret operator()(Args...args, auto...xargs) const {
      return __cb(__hc_fwd_<Args>(args)..., xargs...);
    }
  public:
    CallbackType* __cb = defaultCB;
  };

  template <meta::is_function T>
  struct [[clang::trivial_abi]] DefaultFuncPtr : _DefaultFuncPtrBase<T> {
    using BaseType = _DefaultFuncPtrBase<T>;
    using CallBackType = BaseType::CallbackType;
    using BaseType::defaultCB;
    using BaseType::operator();
  public:
    constexpr bool set(CallBackType* cb) {
      if __expect_false(cb == nullptr)
        return false;
      BaseType::__cb = cb;
      return true;
    }

    constexpr bool setSafe(CallBackType* cb) {
      if __expect_false(this->isSet())
        return false;
      return this->set(cb);
    }

    __always_inline constexpr bool isSet() const {
      return (BaseType::__cb != BaseType::defaultCB);
    }

    __always_inline explicit constexpr operator bool() const {
      return (BaseType::__cb != BaseType::defaultCB);
    }
  };
} // namespace hc::common
