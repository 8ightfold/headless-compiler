//===- Std/__algorithm/comp.hpp -------------------------------------===//
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
#include <Std/__type_traits/desugars_to.hpp>
#include <Meta/Traits.hpp>

namespace std {

struct __equal_to {
  template <typename T, typename U>
  __abi_hidden constexpr bool
   operator()(const T& t, const U& u) const {
    return t == u;
  }
};

template <typename T, typename U>
__global bool __desugars_to_v<__equal_tag, __equal_to, T, U> = true;

/////////////////////////////////////////////////////////////////////////

template <typename T = void, typename U = T>
struct __less {};

template <>
struct __less<void, void> {
  template <typename T, typename U>
  __abi_hidden constexpr bool
   operator()(const T& t, const U& u) const {
    return t < u;
  }
};

template <typename T>
__global bool __desugars_to_v<__less_tag, __less<>, T, T> = true;

template <class T>
__global bool __desugars_to_v<
  __totally_ordered_less_tag, __less<>, T, T>
    = hc::meta::is_integral<T>;

} // namespace std
