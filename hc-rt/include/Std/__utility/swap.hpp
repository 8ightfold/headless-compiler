//===- Std/__utility/swap.hpp ---------------------------------------===//
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

#include <Common/Features.hpp>
#include <Std/__type_traits/enable_if.hpp>
#include <Meta/Traits.hpp>

namespace std {

template <typename T>
using _SwapResult = std::enable_if_t<
  hc::meta::is_move_constructible<T> &&
  hc::meta::is_move_assignable<T>
>;

template <typename T>
inline __abi_hidden constexpr
_SwapResult<T> swap(T& a, T& b) __noexcept {
  T tmp(__hc_move_(a));
  a = __hc_move_(b);
  b = __hc_move_(tmp);
}

template <typename T, usize N>
inline __abi_hidden constexpr
void swap(T(&a)[N], T(&b)[N]) __noexcept {
  for (usize Ix = 0; Ix < N; ++Ix)
    swap(a[Ix], b[Ix]);
}

} // namespace std
