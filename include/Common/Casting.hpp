//===- Common/Casting.hpp -------------------------------------------===//
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

#include "Features.hpp"
#include "Fundamental.hpp"

namespace hc {
  template <typename U = void, typename T>
  __always_inline U* ptr_cast(T* t) __noexcept {
    if constexpr(!__is_void(__remove_const(T))) {
      return reinterpret_cast<U*>(t);
    } else {
      return static_cast<U*>(t);
    }
  }

  template <typename U = void>
  __always_inline U* ptr_cast(uptr i) __noexcept {
    return reinterpret_cast<U*>(i);
  }
} // namespace hc
