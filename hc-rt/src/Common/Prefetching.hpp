//===- Common/Prefetching.hpp ---------------------------------------===//
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

#include "MemUtils.hpp"

#ifndef  _HC_SOFTWARE_PREFETCH
# define _HC_SOFTWARE_PREFETCH 0
#endif // _HC_SOFTWARE_PREFETCH

namespace hc::rt {
  template <usize Count = 1>
  static constexpr usize cacheLinesSize = 64 * Count;

  enum class PrefetchMode : int {
    Read  = 0,
    Write = 1,
  };

  enum class Locality : int {
    None = 0,
    Low  = 1,
    High = 2,
    Max  = 3,
  };
} // namespace hc::rt
