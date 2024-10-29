//===- Common/MMatch.hpp --------------------------------------------===//
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

namespace hc {
  /// Match adaptor, simplifies some stuff.
  /// Use like `MMatch(V).is(arg1, arg2, ...)`.
  template <typename T>
  struct MMatch {
    using Type = meta::RemoveCVRef<T>;
    constexpr MMatch(T V) : V(V) { }
  public:
    constexpr bool is(auto&& R) const {
      return (V == __hc_fwd(R));
    }

    [[gnu::flatten]] constexpr bool 
      is(auto&& R, auto&&...RR) const
     requires(sizeof...(RR) > 0) {
      return (is(__hc_fwd(R)) || 
        ... || is(__hc_fwd(RR)));
    }

    inline constexpr bool isnt(auto&& R, auto&&...RR) const {
      return !is(__hc_fwd(R), __hc_fwd(RR)...);
    }

    constexpr bool in(auto&& low, auto&& high) const {
      return (__hc_fwd(low) <= V) && (V < __hc_fwd(high));
    }

    constexpr bool iin(auto&& low, auto&& high) const {
      return (__hc_fwd(low) <= V) && (V <= __hc_fwd(high));
    }

  public:
    T V;
  };

  template <typename T> 
  MMatch(T&&)  -> MMatch<T>;
} // namespace hc
