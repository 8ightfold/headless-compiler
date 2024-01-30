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

namespace hc {
namespace common {
  template <bool B, typename T, typename F>
  using __conditional_t = __type_pack_element<B, T, F>;  

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
  concept __all_same = (__common_is_same(T, TT) && ...);

  template <typename T, typename...TT>
  concept __any_same = (__common_is_same(T, TT) || ...);

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

#undef __common_is_same
