//===- Common/StaticVec.hpp -----------------------------------------===//
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
//  A vector type with a statically sized array backing.
//
//===----------------------------------------------------------------===//

#pragma once

#include "Fundamental.hpp"
#include "Memory.hpp"

namespace hc::common {
  template <typename T, usize BufferSize>
  struct StaticVec {
    using SelfType = StaticVec;
    using Type = T;
    static constexpr usize underlyingSize = __sizeof(T) * BufferSize;
  public:

  private:
    alignof(T) u8 __buffer[underlyingSize];
    usize __size = 0;
  };
} // namespace hc::common
