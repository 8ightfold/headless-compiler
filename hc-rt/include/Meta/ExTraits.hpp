//===- Meta/ExTraits.hpp --------------------------------------------===//
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
#include "_FnTraits.hpp"
#include "_PackTraits.hpp"
#include "Objects.hpp"

HC_HAS_REQUIRED(builtin, __type_pack_element);
HC_HAS_REQUIRED(builtin, __make_integer_seq);

#define _HC_UNARY_OP(name, op) \
 template <typename T> \
 concept __has_unary_##name = \
requires(T t) { op t; };
#define _HC_BINARY_OP(name, op) \
 template <typename T, typename U = T> \
 concept __has_binary_##name = \
requires(T t, U u) { t op u; };

//======================================================================//
// Uniqueness
//======================================================================//

namespace hc::meta {

template <typename...TT>
struct _TAllUnique {
  template <typename = void>
  static constexpr TySeq<TT...> AsTySeq() 
  { return { }; }
};

template <typename...TT, typename U>
requires(!__any_same<U, TT...>)
_TAllUnique<TT..., U> operator->*(
 _TAllUnique<TT...>, _TAllUnique<U>);

template <typename...TT, typename U>
requires __any_same<U, TT...>
_TAllUnique<TT...> operator->*(
 _TAllUnique<TT...>, _TAllUnique<U>);

template <typename...TT>
using __unique_list_t = decltype(
  (_TAllUnique<>{} ->* ... ->* _TAllUnique<TT>{}).AsTySeq());

template <typename...TT>
concept __all_unique = is_same<
  TySeq<TT...>, __unique_list_t<TT...>>;

//////////////////////////////////////////////////////////////////////////

template <typename Set, typename Subset>
struct _TIsSubset {
  $compile_failure(Set,
    "Not a valid type sequence!");
};

template <typename...Set, typename...Subset>
struct _TIsSubset<TySeq<Set...>, TySeq<Subset...>> {
  static_assert(__all_unique<Set...>, "Invalid set!");
  static constexpr usize setSize = sizeof...(Set);
  static constexpr bool value =
    (setSize >= sizeof...(Subset))
    && (setSize == __unique_list_t<Set..., Subset...>::size);
};

template <class Set, typename...Subset>
concept __is_subset = _TIsSubset<Set, TySeq<Subset...>>::value;

//======================================================================//
// Function Stuff
//======================================================================//

template <typename T, typename...TT>
struct _CommonReturn2 {
  using _F = __decay(T);
  using Type = meta::__conditional_t<
    __all_convertible<_F, __decay(TT)...>, _F, void>;
};

template <bool B, typename...>
struct _CommonReturn {
  using Type = void;
};

template <typename T, typename...TT>
struct _CommonReturn<true, T, TT...> {
#if __has_builtin(__builtin_common_type)
  using Type = __builtin_common_type(T, TT...);
#else
  using Type = __conditional_t<
    __all_same<T, TT...>, T, 
    typename _CommonReturn2<T, TT...>::Type
  >;
#endif
};

template <typename F, typename...TT>
using __common_return_t = typename
  _CommonReturn<(sizeof...(TT) > 0), 
  __return_t<F, TT>...>::Type;

//======================================================================//
// Operator Stuff
//======================================================================//

_HC_UNARY_OP(plus, +)
_HC_UNARY_OP(minus, -)
_HC_UNARY_OP(deref, *)
_HC_UNARY_OP(addrof, &)
_HC_BINARY_OP(pmem, ->*)

_HC_UNARY_OP(inc, ++)
_HC_UNARY_OP(dec, --)
_HC_BINARY_OP(add, +)
_HC_BINARY_OP(sub, -)
_HC_BINARY_OP(mul, *)
_HC_BINARY_OP(div, /)
_HC_BINARY_OP(mod, %)

_HC_BINARY_OP(eq, ==)
_HC_BINARY_OP(ne, !=)
_HC_BINARY_OP(gt, >)
_HC_BINARY_OP(lt, <)
_HC_BINARY_OP(ge, >=)
_HC_BINARY_OP(le, <=)

_HC_UNARY_OP(not, !)
_HC_BINARY_OP(and, &&)
_HC_BINARY_OP(or, ||)
// Sadly no logical xor

_HC_UNARY_OP(bitnot, ~)
_HC_BINARY_OP(bitand, &)
_HC_BINARY_OP(bitor,  |)
_HC_BINARY_OP(bitxor, ^)
_HC_BINARY_OP(shl, <<)
_HC_BINARY_OP(shr, >>)

_HC_BINARY_OP(equals, =)
_HC_BINARY_OP(add_eq, +=)
_HC_BINARY_OP(sub_eq, -=)
_HC_BINARY_OP(mul_eq, *=)
_HC_BINARY_OP(div_eq, /=)
_HC_BINARY_OP(mod_eq, %=)
_HC_BINARY_OP(and_eq, &=)
_HC_BINARY_OP(or_eq,  |=)
_HC_BINARY_OP(shl_eq, <<=)
_HC_BINARY_OP(shr_eq, >>=)

template <typename T, typename...Args>
concept __has_op_call = 
 requires(T t, Args...args) { t(__hc_fwd(args)...); };

template <typename T, typename U>
concept __has_op_subscript = 
 requires(T t, U u) { t[u]; };

template <typename T, typename...Args>
concept __has_op_arrow = __is_pointer(T) ||
 requires(T t) { t.operator->(); };

} // namespace hc::meta

#undef __common_is_same
#undef _HC_UNARY_OP
#undef _HC_BINARY_OP
