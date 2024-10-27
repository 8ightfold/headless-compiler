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

#include <Common/Fundamental.hpp>
#include <Meta/Traits.hpp>

HC_HAS_BUILTIN(clzll);
HC_HAS_BUILTIN(popcount);
HC_HAS_BUILTIN(popcountll);

namespace hc::common {

/// Evaluates to `floor(log2(V))`;
__global u64 bit_log2(u64 V) noexcept {
  // Only check when constant evaluating.
  __hc_cxassert(V != 0);
  const auto LLU = static_cast<unsigned long long>(V);
  // Otherwise, return 0.
  return __expect_false(V == 0) ? 0 :
    __bitsizeof(unsigned long long) 
      - __builtin_clzll(LLU) - 1ull;
}

template <meta::is_integral Int>
__global auto popcnt(Int V) noexcept {
  using Type = meta::MakeUnsigned<Int>;
  if constexpr (sizeof(Int) == 8) {
    // No intermediate value because the rep will be the same.
    return (Type) __builtin_popcountll(Type(V));
  } else {
    // Cast to this first to avoid weird conversions.
    const Type I = static_cast<Type>(V);
    return (Type) __builtin_popcount(static_cast<u32>(I));
  }
}

} // namespace hc::common
