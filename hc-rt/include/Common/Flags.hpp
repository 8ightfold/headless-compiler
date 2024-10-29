//===- Common/Flags.hpp ---------------------------------------------===//
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

#include <Common/FastMath.hpp>
#include <Common/Fundamental.hpp>
#include <Common/Casting.hpp>

namespace hc::common {

template <usize Mask, typename Int>
constexpr bool has_flagval(Int V) noexcept {
  static_assert(popcnt(Mask) == 1);
  const auto cmp = int_cast<usize>(V);
  return (cmp & usize(Mask)) != 0U;
}

template <usize I, typename Int>
constexpr bool has_flag(Int V) noexcept {
  static_assert(I < __bitsizeof(Int));
  const auto cmp = int_cast<usize>(V);
  return (cmp & (1ULL << I)) != 0U;
}

template <usize I, typename Int = usize>
constexpr Int get_bitmask_for() noexcept {
  static_assert(I < __bitsizeof(Int));
  return static_cast<Int>(~(1ULL << I));
}

template <usize I>
inline constexpr usize get_bitmask() noexcept {
  return get_bitmask_for<I, usize>();
}

template <usize I, typename Int>
constexpr bool has_flag_and_bitmask(Int& V) noexcept {
  if (!has_flag<I>(V))
    return false;
  V &= get_bitmask_for<I, Int>();
  return true;
}

//////////////////////////////////////////////////////////////////////////
// Mask Value

template <usize I> struct BitMask {
  static_assert(I < __bitsizeof(usize),
    "Invalid mask offset.");
public:
  template <meta::is_integral_ex Int>
  constexpr operator Int() const {
    return get_bitmask_for<I, Int>();
  }

  template <meta::is_integral_ex Int>
  constexpr Int& operator()(Int& V) const {
    constexpr Int mask = get_bitmask_for<I, Int>();
    return (V &= mask);
  }
};

template <usize I>
__global BitMask<I> bitmask {};

} // namespace hc::common
