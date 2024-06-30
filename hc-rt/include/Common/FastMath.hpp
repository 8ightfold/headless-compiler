//===- Common/FastMath.hpp ------------------------------------------===//
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
//  Defines fast algorithms for commonly used features like log2.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Meta/Traits.hpp>

HC_HAS_BUILTIN(clzll);

namespace hc::common {
  /// Evaluates to `floor(log2(V))`;
  constexpr u64 log2(u64 V) {
    // Only check when constant evaluating.
    __hc_cxassert(V != 0);
    const auto LLU = static_cast<unsigned long long>(V);
    // Otherwise, return 1.
    return __expect_false(V == 0) ? 1 :
      __bitsizeof(unsigned long long) 
        - __builtin_clzll(LLU) - 1ULL;
  }
} // namespace hc::common
