//===- Common/EnumArray.hpp -----------------------------------------===//
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
//
//  An object which can be indexed using enums. Use when enum values
//  are relatively contiguous, otherwise you get a lot of empty slots.
//
//===----------------------------------------------------------------===//

#pragma once

#include "Fundamental.hpp"

namespace hc::common {
  template <typename T, typename E,
    E MaxValue = E::MaxValue,
    typename UType = __underlying_type(E),
    UType N = static_cast<UType>(MaxValue) + 1>
  struct EnumArray {
    using Enum = E;
    using Type = T;
    static constexpr UType __size = N;
  public:
    static constexpr UType Size() 
    { return __size; }

    constexpr T& operator[](E I) {
      __hc_invariant(UType(I) < N);
      return this->__data[UType(I)];
    }

    constexpr const T& operator[](E I) const {
      __hc_invariant(UType(I) < N);
      return this->__data[UType(I)];
    }

  public:
    T __data[N] { };
  };
} // namespace hc::common
