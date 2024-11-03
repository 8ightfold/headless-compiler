//===- Common/Handle.hpp --------------------------------------------===//
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
//  This file implements a wrapper type that allows for type-safe
//  uses of generally unsafe objects (eg. void*, int, etc).
//
//===----------------------------------------------------------------===//

#pragma once

#include "_HandleAttr.hpp"

namespace hc {

/// A wrapper type used to differentiate objects for type safety.
/// @tparam T The underlying type of the handle.
/// @tparam ID The unique identifier for the type.
template <typename T, typename ID, typename...AA>
struct __empty_bases Handle
#if _HC_DEDUCING_THIS
  : _HandleAttr<AA>...
#else
  : _HandleAttr<Handle<T, ID, AA...>, AA>...
#endif
{
  using SelfType = Handle;
  using Type = T;
  using IDType = ID;

  template <typename Attr>
#if _HC_DEDUCING_THIS
  using _BoundAttr = _HandleAttr<Attr>;
#else
  using _BoundAttr = _HandleAttr<SelfType, Attr>;
#endif

public:
  __ndbg_inline static constexpr IDType New(auto&&...args) __noexcept {
    if constexpr(requires { T::New(__hc_fwd(args)...); })
      return { _BoundAttr<AA>{}..., T::New(__hc_fwd(args)...) };
    else
      return IDType::NewRaw(__hc_fwd(args)...);
  }

  __ndbg_inline static constexpr IDType NewRaw(auto&&...args) __noexcept {
    return { _BoundAttr<AA>{}..., T{__hc_fwd(args)...} };
  }

  constexpr decltype(auto) get(this auto&& self) {
    return __hc_fwd(self).__data;
  }

public:
  T __data { };
};

//////////////////////////////////////////////////////////////////////////

template <typename HandleType>
__always_inline constexpr HandleType make_handle(auto&&...args) {
  return HandleType::New(__hc_fwd(args)...);
}

template <typename H, typename Group>
concept __handle_in_group = requires(H& h) {
#if _HC_DEDUCING_THIS
  static_cast<_HandleAttr<HandleAttr::InGroup<Group>>&>(h);
#else
  static_cast<_HandleAttr<H, HandleAttr::InGroup<Group>>&>(h);
#endif
};

template <typename H, typename...Groups>
concept handle_in_group = 
  ((sizeof...(Groups) == 0) ||
    ... || __handle_in_group<H, Groups>);

$Handle(__eg_handle_, void*, Boolean, Pointer);
static_assert(sizeof(__eg_handle_) == sizeof(void*));

//////////////////////////////////////////////////////////////////////////
// Casting

template <typename To, typename From>
concept handle_can_static_cast = requires(From& from) {
  static_cast<To>(from);
};

template <typename To, typename From,
  typename ID, typename...AA>
__ndbg_inline constexpr To
 handle_cast(const Handle<From, ID, AA...>& from) {
  static_assert(__is_convertible(From, To),
    "Underlying type is not convertible.");
  return static_cast<To>(from.__data);
}

template <typename To, typename From,
  typename ID, typename...AA>
__ndbg_inline constexpr To
 handle_cast_ex(const Handle<From, ID, AA...>& from) {
  static_assert(handle_can_static_cast<From, To>,
    "Underlying type is not convertible.");
  return static_cast<To>(from.__data);
}

} // namespace hc
