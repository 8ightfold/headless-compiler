//===- Common/Align.hpp ---------------------------------------------===//
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

#include <Meta/Traits.hpp>

namespace hc::common {
  struct Align {
    template <usize BitMax>
    __always_inline static constexpr usize
     __Up(usize i) __noexcept {
      usize n = i;
      n |= n >> 1;
      n |= n >> 2;
      n |= n >> 4;
      if constexpr (BitMax > 8)
        n |= n >> 8;
      if constexpr (BitMax > 16)
        n |= n >> 16;
      if constexpr (BitMax > 32)
        n |= n >> 32;
      return n + 1;
    }

    template <meta::is_integral T, typename CastType = __make_unsigned(T)>
    static constexpr CastType Up(T i) __noexcept {
      if __expect_false(i == 0) return 1;
      const usize n = __Up<__bitsizeof(T)>(usize(i));
      return static_cast<CastType>(n);
    }
  };
} // namespace hc::common
