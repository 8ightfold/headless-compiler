//===- Meta/Traits.hpp ----------------------------------------------===//
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

#include <Common/Fundamental.hpp>

//======================================================================//
// Concepts
//======================================================================//

namespace hc::meta {
  template <typename T>
  concept is_void = __is_void(T);

  template <typename T>
  concept not_void = !__is_void(T);

  template <typename T, typename U>
  concept is_same = __is_same(T, U) && __is_same(U, T);

  template <typename T>
  concept is_integral = __is_integral(T);

  template <typename T>
  concept is_signed = is_integral<T> && __is_signed(T);

  template <typename T>
  concept is_unsigned = is_integral<T> && __is_unsigned(T);

  template <typename T>
  concept is_float = __is_floating_point(T);

  template <typename T>
  concept is_array = __is_array(T);

  template <typename T>
  concept is_enum = __is_enum(T);

  template <typename T>
  concept is_union = __is_union(T);

  template <typename T>
  concept is_struct = __is_class(T);

  // Qualifiers

  template <typename T>
  concept is_ref = __is_reference(T);

  template <typename T>
  concept not_ref = !__is_reference(T);

  template <typename T>
  concept is_ptr = __is_pointer(T);

  template <typename T>
  concept not_ptr = !__is_pointer(T);

  template <typename T>
  concept is_const = __is_const(T);

  template <typename T>
  concept not_const = !__is_const(T);

  template <typename T>
  concept is_volatile = __is_volatile(T);

  template <typename T>
  concept not_volatile = !__is_volatile(T);

  template <typename T>
  concept is_cv = is_const<T> || is_volatile<T>;

  template <typename T>
  concept not_cv = not_const<T> && not_volatile<T>;

  // Complex

  template <typename T>
  concept is_trivial = __is_trivial(T);

  template <typename T>
  concept is_trivially_copyable = __is_trivially_copyable(T);

  template <typename T>
  concept is_trivially_destructible = __is_trivially_destructible(T);

  template <typename T>
  concept is_object = __is_object(T);

  template <typename T, typename U>
  concept is_same_size = 
    (__sizeof(T) == __sizeof(U)) &&
    (__sizeof(U) == __sizeof(T));

} // namespace hc::meta

//======================================================================//
// Types
//======================================================================//

namespace hc::meta {
  template <typename T>
  using Decay = __decay(T);

  template <typename T>
  using UnderlyingType = __underlying_type(T);

  template <typename T>
  using AddPointer = __add_pointer(T);

  template <typename T>
  using RemoveConst = __remove_const(T);

  template <typename T>
  using RemoveVolatile = __remove_volatile(T);

  template <typename T>
  using RemoveRef = __remove_reference_t(T);

  template <typename T>
  using RemoveCV = __remove_cv(T);

  template <typename T>
  using RemoveCVRef = __remove_cvref(T);

  template <bool B, typename T, typename F>
  using __conditional_t = __type_pack_element<B, F, T>;  

  template <usize I, typename...TT>
  using __selector_t = __type_pack_element<I, TT...>;
} // namespace hc::meta
