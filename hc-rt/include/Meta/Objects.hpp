//===- Meta/Objects.hpp ---------------------------------------------===//
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

HC_HAS_REQUIRED(builtin, __type_pack_element);
HC_HAS_REQUIRED(builtin, __make_integer_seq);

#define $I(n) ::hc::__i<n>

//======================================================================//
// *Node
//======================================================================//

namespace hc::meta {
  template <typename T>
  struct TyNode {
    using Type = T;
  };

  template <usize I>
  struct IdxNode {
    static constexpr usize value = I;
    constexpr operator usize() const { return I; }
  };

  template <typename T, T V>
  struct ValNode {
    static constexpr T value = V;
    constexpr operator T() const { return V; }
  };

  template <auto V>
  using AutoNode = ValNode<decltype(V), V>;
} // namespace hc::meta

//======================================================================//
// *Seq
//======================================================================//

namespace hc::meta {
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
  using make_idxseq = __make_integer_seq<IntSeq, usize, N>;
} // namespace hc::meta

//======================================================================//
// Pull In
//======================================================================//

namespace hc::common {
  using meta::TyNode;
  using meta::IdxNode;
  using meta::TySeq;
  using meta::IntSeq;
  using meta::IdxSeq;
  using meta::make_idxseq;
} // namespace hc::common

namespace hc {
  template <usize I>
  __global meta::IdxNode<I> __i { };
} // namespace hc
