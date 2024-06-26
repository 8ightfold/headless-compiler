//===- Parcel/Common.hpp --------------------------------------------===//
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
//  This file includes core utilities from hc::common, and provides a
//  simple alias for usage.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Fundamental.hpp>
#include <Common/Align.hpp>

namespace hc::parcel {
  template <meta::is_integral Int>
  __always_inline constexpr
   Int popcnt(Int I) noexcept {
    if constexpr (sizeof(I) == 8)
      return __builtin_popcountll(I);
    else
      return __builtin_popcount(I);
  }
} // namespace hc::parcel
