//===- Parcel/BitSet.hpp --------------------------------------------===//
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

#include "Common.hpp"
#include "BitList.hpp"

namespace hc::parcel {
  template <usize N>
  struct BitSet : BitList<N, u64> {
    static_assert(N != 0);
    using SelfType = BitSet;
    using BaseType = BitList<N, u64>;
    using BaseType::__perIx;
    using BaseType::__count;
    using BaseType::bitCount;
  private:
    // Hide irrelevant/conflicting stuff.
    using BaseType::countActive;
    using BaseType::countInactive;
    using BaseType::flip;
    using BaseType::reset;
    using BaseType::operator[];
  public:
    using BaseType::set;
    using BaseType::get;

    inline constexpr bool
     test(usize I) const {
      return get(I);
    }

    inline constexpr void flip() {
      for (usize I = 0; I < __count; ++I)
        BaseType::__data[I] 
          = ~BaseType::__data[I];
    }

    inline constexpr void reset() {
      (void) BaseType::reset();
    }

    inline constexpr bool operator==(
      const SelfType& R) const = default;
  };

  template <usize N>
  using ALBitSet = BitSet<CC::Align::UpEq(N)>;
} // namespace hc::parcel
