//===- Common/Casting.hpp -------------------------------------------===//
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

#include <Meta/Traits.hpp>

//======================================================================//
// Basic Casts
//======================================================================//

namespace hc {
  template <typename U = void, typename T>
  __ndbg_inline U* ptr_cast(T* t) __noexcept {
    if constexpr (meta::not_void<meta::RemoveConst<T>>) {
      return reinterpret_cast<U*>(t);
    } else {
      return static_cast<U*>(t);
    }
  }

  template <typename U = void>
  __ndbg_inline U* ptr_cast(uptr i) __noexcept {
    return reinterpret_cast<U*>(i);
  }

  template <typename U = void, typename T>
  __ndbg_inline U* ptr_castex(T* t) __noexcept {
    if constexpr (meta::not_void<meta::RemoveConst<T>>) {
      return (U*)(t);
    } else {
      return static_cast<U*>(t);
    }
  }

  template <typename U = void>
  __ndbg_inline U* ptr_castex(uptr i) __noexcept {
    return (U*)(i);
  }
} // namespace hc

//======================================================================//
// isa/dyn_cast
//======================================================================//

namespace hc {
  template <typename Base, typename To>
  concept __has__isa = 
    requires(Base B) { B.template __isa<To>(); };

  template <typename Base, typename To>
  concept __has__dyn_cast = 
    requires(Base B) { B.template __dyn_cast<To>(); };
  
  template <typename To, typename Base>
  requires __has__isa<Base, To>
  __ndbg_inline constexpr bool isa(Base&& B) __noexcept {
    return static_cast<bool>(
      __hc_fwd(B).template __isa<To>());
  }

  template <typename To, typename Base>
  requires __has__dyn_cast<Base, To>
  __ndbg_inline decltype(auto) dyn_cast(Base&& B) __noexcept {
    return __hc_fwd(B).template __dyn_cast<To>();
  }
} // namespace hc

//======================================================================//
// Handles
//======================================================================//

namespace hc {
  // TODO: Move into Handle.hpp?
  template <typename T, 
    typename ID, typename...AA>
  struct Handle;

  template <typename To, typename From,
    typename ID, typename...AA>
  __ndbg_inline constexpr To
   handle_cast(const Handle<From, ID, AA...>& from) {
    static_assert(__is_convertible(From, To),
      "Underlying type is not convertible.");
    return static_cast<To>(from.__data);
  }
} // namespace hc

//======================================================================//
// Underlying
//======================================================================//

namespace hc {
  template <meta::is_enum E>
  __ndbg_inline constexpr auto underlying_cast(E e) {
    return static_cast<meta::UnderlyingType<E>>(e);
  }

  template <typename T>
  __ndbg_inline constexpr T underlying_cast(T&& t) {
    return __hc_fwd(t);
  }
} // namespace hc

//======================================================================//
// Reinterpreting
//======================================================================//

namespace hc {
  template <typename U, typename T>
  requires meta::is_same_size<T, U>
  U pun_cast(const T& V) __noexcept {
    U u;
    __builtin_memcpy_inline(&u, &V, sizeof(U));
    return u;
  }
} // namespace hc
