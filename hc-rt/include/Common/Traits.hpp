//===- Common/Traits.hpp --------------------------------------------===//
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

#define __common_is_same(t, u) (__is_same(t, u) && __is_same(u, t))
#define $I(n) ::hc::__i<n>

#define _HC_UNARY_OP(name, op) \
 template <typename T> \
 concept __has_unary_##name = \
  requires(T t) { op t; };
#define _HC_BINARY_OP(name, op) \
 template <typename T, typename U = T> \
 concept __has_binary_##name = \
  requires(T t, U u) { t op u; };

namespace hc {
namespace common {
  template <bool B, typename T, typename F>
  using __conditional_t = __type_pack_element<B, F, T>;  

  template <usize I, typename...TT>
  using __selector_t = __type_pack_element<I, TT...>;

  template <typename T>
  struct TyNode {
    using Type = T;
  };

  template <usize I>
  struct IdxNode {
    static constexpr usize value = I;
  };
} // namespace common

  template <usize I>
  __global common::IdxNode<I> __i { };
} // namespace hc

//=== *Seq ===//
namespace hc::common {
  template <typename...TT>
  struct TySeq {
    static constexpr auto size = sizeof...(TT);
  };

  template <auto...VV>
  struct ValSeq {
    static constexpr auto size = sizeof...(VV);
  };

  // Integer sequences

  template <typename I, I...NN>
  struct IntSeq {
    using Type = I;
    static constexpr auto size = sizeof...(NN);
  };

  template <usize...NN>
  using IdxSeq = IntSeq<usize, NN...>;

  template <typename I, I N>
  using make_intseq = __make_integer_seq<IntSeq, I, N>;

  template <usize N>
  using make_idxseq = make_intseq<usize, N>;
} // namespace hc::common

//=== Uniqueness ===//
namespace hc::common {
  template <typename T, typename...TT>
  concept __all_same = (true && ... && __common_is_same(T, TT));

  template <typename T, typename...TT>
  concept __any_same = (false || ... || __common_is_same(T, TT));

  // All Unique

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
  concept __all_unique = __common_is_same(
    TySeq<TT...>, __unique_list_t<TT...>);
} // namespace hc::common

//=== Function Stuff ===//
namespace hc::common {
  template <typename T> 
  __add_rvalue_reference(T) Decl() noexcept {
    static_assert(!__is_same(T, T), 
      "Decl<...> must be called in an unevaluated context.");
  }

  template <typename F, typename...TT>
  using __return_t = decltype(Decl<F>()(Decl<TT>()...));

  template <typename From, typename To>
  concept __convertible = __is_convertible(From, To);

  template <typename To, typename...From>
  concept __all_convertible = (true && ... && __convertible<From, To>);

  // Common Return?

  template <typename T, typename...TT>
  struct _CommonReturn2 {
    using _F = __decay(T);
    using Type = __conditional_t<
      __all_convertible<_F, __decay(TT)...>, _F, void>;
  };

  template <bool B, typename...>
  struct _CommonReturn {
    using Type = void;
  };

  template <typename T, typename...TT>
  struct _CommonReturn<true, T, TT...> {
    using Type = __conditional_t<
      __all_same<T, TT...>, T, 
      typename _CommonReturn2<T, TT...>::Type
    >;
  };

  template <typename F, typename...TT>
  using __common_return_t = typename
    _CommonReturn<(sizeof...(TT) > 0), 
    __return_t<F, TT>...>::Type;
} // namespace hc::common

//=== Operator Stuff ===//
namespace hc::common {
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
} // namespace hc::common

#undef __common_is_same
#undef _HC_UNARY_OP
#undef _HC_BINARY_OP