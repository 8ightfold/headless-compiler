//===- Common/MemUtils.hpp ------------------------------------------===//
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
#include <Common/Memory.hpp>
#include "SSEVec.hpp"

namespace hc::rt {
  template <typename T, typename PtrType>
  constexpr void store(PtrType* dst, T value) {
    static_assert(!__is_void(PtrType));
    __builtin_memcpy_inline(dst, &value, sizeof(T));
  }

  template <typename T>
  inline constexpr T splat(u8 value) {
    if constexpr (__is_scalar(T)) {
      return T(~0) / T(0xFF) * T(value);
    } else if constexpr(__is_vector<T>) {
      T out;
      for (usize I = 0; I < sizeof(T); ++I)
        out[I] = value;
      return out;
    } else {
      using U = typename T::Type;
      const auto u = splat<U>(value);
      T out;
      for (usize I = 0; I < T::Size(); ++I)
        out[I] = u;
      return out;
    }
  }
} // namespace hc::rt
