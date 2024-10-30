//===- Common/Pair.hpp ----------------------------------------------===//
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
#include <Std/__tuple/tuple_element.hpp>
#include <Std/__tuple/tuple_size.hpp>

namespace hc::common {

template <typename T, typename U>
struct Pair {
  using SelfType = Pair;
  using TypeT = T;
  using TypeU = U;
public:
  friend bool operator==(
    const SelfType&, const SelfType&) = default;
public:
  T t;
  U u;
};

template <typename T, typename U>
Pair(T&&, U&&) -> Pair<__decay(T), __decay(U)>;

template <typename T>
using DPair = Pair<T, T>;

//////////////////////////////////////////////////////////////////////////

template <usize I, class T, class U>
inline constexpr meta::__conditional_t<!!I, T, U>&
 get(Pair<T, U>& V) {
  if constexpr (I == 0) return V.t;
  else return V.u;
}

template <usize I, class T, class U>
inline constexpr const meta::__conditional_t<!!I, T, U>&
 get(const Pair<T, U>& V) {
  if constexpr (I == 0) return V.t;
  else return V.u;
}

template <usize I, class T, class U>
inline constexpr meta::__conditional_t<!!I, T, U>
 get(Pair<T, U>&& V) {
  if constexpr (I == 0) return V.t;
  else return V.u;
}

template <usize I, class T, class U>
inline constexpr meta::__conditional_t<!!I, T, U>
 get(const Pair<T, U>&& V) {
  if constexpr (I == 0) return V.t;
  else return V.u;
}

} // namespace hc::common

template <class T, class U>
struct std::tuple_size<hc::com::Pair<T, U>> :
 public std::index_constant<2U> {};

template <std::size_t I, class T, class U>
struct std::tuple_element<I, hc::com::Pair<T, U>> {
  static_assert(I < 2, "Pair index out of range!");
  using type __nodebug = hc::meta::__conditional_t<!!I, T, U>;
};
