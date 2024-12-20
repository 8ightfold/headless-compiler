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
#include <Std/__type_traits/conditional.hpp>

namespace hc::meta {
// template <typename> __global bool __is_floating_point       = false;
// template <> __global bool __is_floating_point<float>        = true;
// template <> __global bool __is_floating_point<double>       = true;
// template <> __global bool __is_floating_point<long double>  = true;
} // namespace hc::meta

//======================================================================//
// Concepts
//======================================================================//

namespace hc::meta {

template <typename T>
concept is_void = __is_void(T);

template <typename T>
concept not_void = !is_void<T>;

template <typename T, typename U>
concept __iis_same = __is_same(T, U);

template <typename T, typename U>
concept is_same = __iis_same<T, U> && __iis_same<U, T>;

template <typename T, typename U>
concept not_same = !is_same<T, U>;

template <typename T>
concept is_integral = __is_integral(T);

template <typename T>
concept is_signed = is_integral<T> && __is_signed(T);

template <typename T>
concept is_unsigned = is_integral<T> && __is_unsigned(T);

template <typename T>
concept is_float = __is_floating_point(T);

template <typename T>
concept is_arithmetic = is_integral<T> || is_float<T>;

template <typename T>
concept is_array = __is_array(T);  

template <typename T>
concept is_enum = __is_enum(T);

template <typename T>
concept is_union = __is_union(T);

template <typename T>
concept is_struct = __is_class(T);

template <typename T>
concept is_object = __is_object(T);

template <typename T>
concept is_function = __is_function(T);

/// Checks if type is an integral or enum.
template <typename T>
concept is_integral_ex = is_integral<T> || is_enum<T>;

template <typename T, typename U>
concept __both_signed =
  (is_signed<T> && is_signed<U>) &&
  (is_signed<U> && is_signed<T>);

template <typename T, typename U>
concept __both_unsigned =
  (is_unsigned<T> && is_unsigned<U>) &&
  (is_unsigned<U> && is_unsigned<T>);

template <typename T, typename U>
concept __is_same_sign = __both_signed<T, U> || __both_unsigned<T, U>;

/// Checks if signs are the same.
template <typename T, typename U>
concept is_same_sign = __is_same_sign<T, U> && __is_same_sign<U, T>;

//////////////////////////////////////////////////////////////////////////
// Qualifiers

template <typename T>
concept is_lvalue_ref = __is_lvalue_reference(T);

template <typename T>
concept is_rvalue_ref = __is_rvalue_reference(T);

template <typename T>
concept not_lvalue_ref = !is_lvalue_ref<T>;

template <typename T>
concept not_rvalue_ref = !is_rvalue_ref<T>;

template <typename T>
concept is_ref = __is_reference(T);

template <typename T>
concept not_ref = !is_ref<T>;

template <typename T>
concept is_ptr = __is_pointer(T);

template <typename T>
concept not_ptr = !is_ptr<T>;

template <typename T>
concept is_const = __is_const(T);

template <typename T>
concept not_const = !is_const<T>;

template <typename T>
concept is_volatile = __is_volatile(T);

template <typename T>
concept not_volatile = !is_volatile<T>;

template <typename T>
concept is_cv = is_const<T> || is_volatile<T>;

template <typename T>
concept not_cv = not_const<T> && not_volatile<T>;

template <typename T>
concept only_const = is_const<T> && not_volatile<T>;

template <typename T>
concept only_volatile = not_const<T> && is_volatile<T>;

//////////////////////////////////////////////////////////////////////////
// Complex

template <typename T>
concept is_trivial = __is_trivial(T);

template <typename T>
concept is_standard_layout =  __is_standard_layout(T);

template <typename T, typename...Args>
concept is_trivially_constructible =
  __is_trivially_constructible(T, Args...);

template <typename T>
concept is_trivially_default_constructible = 
  __is_trivially_constructible(T);

template <typename T>
concept is_trivially_copyable = __is_trivially_copyable(T);

template <typename T>
concept is_trivially_movable = 
  __is_trivially_constructible(T, __add_rvalue_reference(T));

template <typename T>
concept is_trivially_destructible = __is_trivially_destructible(T);

template <typename T>
concept is_trivially_relocatable = __is_trivially_relocatable(T);

template <typename T>
concept is_bitwise_cloneable
#if __has_builtin(__is_bitwise_cloneable)
  = __is_bitwise_cloneable(T);
#else
  = is_trivially_relocatable<T>;
#endif

template <typename T, typename...Args>
concept is_constructible = __is_constructible(T, Args...);

template <typename T>
concept is_copy_constructible = 
  is_constructible<T, __add_lvalue_reference(const T)>;

template <typename T>
concept is_move_constructible = 
  is_constructible<T, __add_rvalue_reference(T)>;

template <typename T, typename U>
concept is_assignable = __is_assignable(T, U);

template <typename T>
concept is_copy_assignable = is_assignable<
  __add_lvalue_reference(T), __add_lvalue_reference(const T)>;

template <typename T>
concept is_move_assignable = is_assignable<
  __add_lvalue_reference(T), __add_rvalue_reference(T)>;

//////////////////////////////////////////////////////////////////////////
// Misc.

template <typename T, typename U>
concept __is_same_size = (__sizeof(T) == __sizeof(U));

template <typename T, typename U>
concept is_same_size = __is_same_size<T, U> && __is_same_size<U, T>;

//======================================================================//
// Types
//======================================================================//

template <typename T>
using Decay = __decay(T);

template <typename T>
using MakeSigned = __make_signed(T);

template <typename T>
using MakeUnsigned = __make_unsigned(T);

//////////////////////////////////////////////////////////////////////////
// Enum

template <typename T, bool = false>
struct _IUnderlyingType { using Type = T; };

template <typename T>
struct _IUnderlyingType<T, true> {
  using Type = __underlying_type(T);
};

template <typename T>
using UnderlyingType = typename
  _IUnderlyingType<T, is_enum<T>>::Type;

//////////////////////////////////////////////////////////////////////////
// Add/Remove

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

template <typename T>
using RemovePtr = __remove_pointer(T);

//////////////////////////////////////////////////////////////////////////
// Misc.

template <bool B, typename T, typename F>
using __conditional_t = typename std::conditional<B, T, F>::type;  

template <usize I, typename...TT>
using __selector_t = __type_pack_element<I, TT...>;

} // namespace hc::meta
