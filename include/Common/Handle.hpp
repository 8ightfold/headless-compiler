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

#include "Fundamental.hpp"
#include "Preproc.hpp"

__cldiag(push)
__cldiag(ignored "-Wvariadic-macros")

#define _HC_HANDLE_ATTRIB(name) ::hc::HandleAttr::$PP_rm_parens(name)

/// Defines a new handle `name`.
/// Single attributes are: `Boolean`, `Equality`, `Indexed`, `Invokable`, and `Pointer`.
/// Template attributes are: `InGroup<...>`.
#define $Handle(name, type, attrs...) using name = \
 ::hc::Handle<$PP_rm_parens(type), struct _Hnd_##name##_ \
  __VA_OPT__(,) $PP_mapCL(_HC_HANDLE_ATTRIB, attrs) >

/// Defines a new handle group `name`.
#define $HandleGroup(name) using name = \
 ::hc::_HandleGroup<struct _HGrp_##name##_>

__cldiag(pop)

namespace hc {
  template <typename GroupID>
  struct _HandleGroup {
    using GroupType = GroupID;
    static constexpr bool __isGroup = true;
  };

  $HandleGroup(AllHandles);

  struct HandleAttr {
    struct Boolean;
    struct Equality;
    struct Indexed;
    struct Invokable;
    struct Pointer;

    template <typename Group>
    struct InGroup;
  };

  template <typename D, typename B>
  requires(__is_base_of(B, D))
  __ndbg_inline constexpr D& __href_cast(B* base) {
    return static_cast<D&>(*base);
  }

  template <typename D, typename B>
  requires(__is_base_of(B, D))
  __ndbg_inline constexpr const D& __href_cast(const B* base) {
    return static_cast<const D&>(*base);
  }

  template <typename Base, typename A>
  struct _HandleAttr;

  template <typename Base>
  struct _HandleAttr<Base, HandleAttr::Boolean> {
    __always_inline explicit constexpr operator bool() const {
      const auto& self = __href_cast<Base>(this);
      return static_cast<bool>(self.__data);
    }
  };

  template <typename Base>
  struct _HandleAttr<Base, HandleAttr::Equality> {
    __always_inline friend constexpr auto
     operator==(const Base& lhs, const Base& rhs) {
      return (lhs.__data == rhs.__data);
    }
  };

  template <typename Base>
  struct _HandleAttr<Base, HandleAttr::Indexed> {
    __always_inline constexpr decltype(auto) operator[](auto&& IX) {
      return __href_cast<Base>(this).__data[__hc_fwd(IX)];
    }
    __always_inline constexpr decltype(auto) operator[](auto&& IX) const {
      return __href_cast<Base>(this).__data[__hc_fwd(IX)];
    }
  };

  template <typename Base>
  struct _HandleAttr<Base, HandleAttr::Invokable> {
    __ndbg_inline constexpr decltype(auto) operator()(auto&&...args)& {
      return static_cast<Base&>(*this).__data(__hc_fwd(args)...);
    }
    __ndbg_inline constexpr decltype(auto) operator()(auto&&...args) const& {
      return static_cast<const Base&>(*this).__data(__hc_fwd(args)...);
    }
    __ndbg_inline constexpr decltype(auto) operator()(auto&&...args)&& {
      return static_cast<Base&&>(*this).__data(__hc_fwd(args)...);
    }
    __ndbg_inline constexpr decltype(auto) operator()(auto&&...args) const&& {
      return static_cast<const Base&&>(*this).__data(__hc_fwd(args)...);
    }
  };

  template <typename Base>
  struct _HandleAttr<Base, HandleAttr::Pointer> {
    __always_inline friend constexpr auto
     operator==(const Base& self, nullptr_t) {
      return (self.__data == nullptr);
    }
    __always_inline friend constexpr auto
     operator==(nullptr_t, const Base& self) {
      return (self.__data == nullptr);
    }

    __ndbg_inline constexpr decltype(auto) operator*() {
      auto& self = __href_cast<Base>(this);
      return *self.__data;
    }
    __ndbg_inline constexpr decltype(auto) operator*() const {
      auto& self = __href_cast<Base>(this);
      return *self.__data;
    }

    __ndbg_inline constexpr decltype(auto) operator->() {
      return __href_cast<Base>(this).__data;
    }
    __ndbg_inline constexpr decltype(auto) operator->() const {
      return __href_cast<Base>(this).__data;
    }
  };

  template <typename Base, typename Group>
  struct _HandleAttr<Base, HandleAttr::InGroup<Group>> {
    constexpr void __ingroup(Group) { }
  };
  
  /// A wrapper type used to differentiate objects for type safety.
  /// @tparam T The underlying type of the handle.
  /// @tparam ID The unique identifier for the type.
  template <typename T, typename ID = struct _HandleGID, typename...AA>
  struct Handle : _HandleAttr<Handle<T, ID, AA...>, AA>... {
    using SelfType = Handle;
    using Type = T;
    using IDType = ID;

    template <typename Attr>
    using _BoundAttr = _HandleAttr<SelfType, Attr>;
  public:
    __ndbg_inline static constexpr SelfType New(auto&&...args) __noexcept {
      if constexpr(requires { T::New(__hc_fwd(args)...); })
        return { _BoundAttr<AA>{}..., T::New(__hc_fwd(args)...) };
      else
        return { _BoundAttr<AA>{}..., T{__hc_fwd(args)...} };
    }

    __ndbg_inline static constexpr SelfType NewRaw(auto&&...args) __noexcept {
      return { _BoundAttr<AA>{}..., T{__hc_fwd(args)...} };
    }
  
  public:
    T __data { };
  };

  template <typename HandleType>
  __always_inline constexpr HandleType make_handle(auto&&...args) {
    return HandleType::New(__hc_fwd(args)...);
  }

  template <typename H, typename Group>
  concept handle_in_group = requires(H& h) {
    static_cast<_HandleAttr<H, HandleAttr::InGroup<Group>>&>(h);
  };

  $Handle(__eg_handle_, void*, Boolean, Pointer);
  static_assert(sizeof(__eg_handle_) == sizeof(void*));
} // namespace hc
