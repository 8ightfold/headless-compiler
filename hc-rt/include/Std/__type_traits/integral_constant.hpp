//===- Std/__type_traits/integral_constant.hpp ----------------------===//
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

#include <Std/cstddef>

namespace std {

template <typename T, T V>
struct integral_constant {
  static constexpr T value = V;
  using value_type = T;
  using type = integral_constant;
  __abi_hidden constexpr operator T() const noexcept { return V; }
  __abi_hidden constexpr T operator()() const noexcept { return V; }
};

template <size_t I>
using index_constant = integral_constant<size_t, I>;

template <bool B>
using bool_constant = integral_constant<bool, B>;

using true_type  = bool_constant<true>;
using false_type = bool_constant<false>;

} // namespace std
