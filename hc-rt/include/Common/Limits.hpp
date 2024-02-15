//===- Common/Limits.hpp --------------------------------------------===//
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

#include "Fundamental.hpp"

HC_HAS_REQUIRED(builtin, __is_signed);
HC_HAS_REQUIRED(builtin, __is_unsigned);

//=== Max ===//
namespace hc {
  template <typename T>
  struct _Limits {
    $compile_failure(_Limits, "Undefined limit type.")
  };

  template <typename T>
  struct _Limits<const T> : _Limits<T> { };

  template <typename T>
  struct _Limits<volatile T> : _Limits<T> { };

  // Implementation

  template <typename T>
  requires(__is_signed(T))
  struct _Limits<T> {
    static constexpr T max = ~(T(1) << (__bitsizeof(T) - 1));
    static constexpr T min = -max - T(1);
    static constexpr T lowest = min;
  };

  template <typename T>
  requires(__is_unsigned(T))
  struct _Limits<T> {
    static constexpr T max = ~T(0U);
    static constexpr T min = T(0U);
    static constexpr T lowest = min;
  };

  template <>
  struct _Limits<f16> {
    static constexpr f16 max = __FLT16_MAX__;
    static constexpr f16 min = __FLT16_MIN__;
    static constexpr f16 lowest = -min;
  };

  template <>
  struct _Limits<f32> {
    static constexpr f32 max = __FLT_MAX__;
    static constexpr f32 min = __FLT_MIN__;
    static constexpr f32 lowest = -min;
  };

  template <>
  struct _Limits<f64> {
    static constexpr f64 max = __DBL_MAX__;
    static constexpr f64 min = __DBL_MIN__;
    static constexpr f64 lowest = -min;
  };

  template <typename T>
  __global T Max = _Limits<T>::max;

  template <typename T>
  __global T Min = _Limits<T>::min;

  template <typename T>
  __global T Lowest = _Limits<T>::lowest;
} // namespace hc
