//===- Std/__algorithm/max.hpp --------------------------------------===//
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
#include <Std/__algorithm/comp.hpp>

namespace std {

template <typename T, class Cmp>
[[nodiscard]] inline __abi_hidden constexpr const T&
 max(__lifetimebound const T& lhs, __lifetimebound const T& rhs, Cmp cmp) {
  return cmp(lhs, rhs) ? rhs : lhs;
}

template <typename T>
[[nodiscard]] inline __abi_hidden constexpr const T&
 max(__lifetimebound const T& lhs, __lifetimebound const T& rhs) {
  return std::max(lhs, rhs, __less<>());
}

// TODO: initializer_list?

} // namespace std
