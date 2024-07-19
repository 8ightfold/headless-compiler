//===- Common/Function.hpp ------------------------------------------===//
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
//  This file implements a non-owning wrapper around functions and
//  functors. Avoid using this as a method for storage.
//
//===----------------------------------------------------------------===//

#pragma once

#include "Fundamental.hpp"
#include <Meta/_FnTraits.hpp>

namespace hc::common {
  template <typename CB, typename Ret, typename...Args>
  concept __is_invokable_ret = meta::is_void<Ret> ||
   requires(meta::RemoveRef<CB>& cb, Args...args) {
    { cb(__hc_fwd_<Args>(args)...) } -> meta::__convertible<Ret>;
  };

  template <typename F>
  struct [[gsl::Pointer]] Function;

  template <typename Ret, typename...Args>
  struct [[gsl::Pointer]] Function<Ret(Args...)> {
    using SelfType = Function;
    using CallbackType = Ret(*)(void*, Args...);

  private:
    template <typename CB>
    static Ret Invoker(void* instance, Args...args) {
      auto& cb = *static_cast<CB*>(instance);
      return static_cast<Ret>(
        cb(__hc_fwd_<Args>(args)...));
    }

  public:
    Function() = default;
    Function(nullptr_t) = delete;

    template <__is_invokable_ret<Ret, Args...> CB>
    Function(CB&& cb) 
      requires meta::not_same<meta::RemoveCVRef<CB>, SelfType> :
     callback(&SelfType::Invoker<meta::RemoveRef<CB>>),
     instance(static_cast<void*>(&cb)) { }

    Ret operator()(Args...args) const {
      __hc_invariant(!!this->callback);
      return callback(instance, __hc_fwd_<Args>(args)...);
    }

    constexpr operator bool() const {
      return !!this->callback;
    }

  private:
    CallbackType callback = nullptr;
    void* instance = nullptr;
  };
} // namespace hc::common
