//===- Meta/Traits.hpp ----------------------------------------------===//
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

#include <Common/Fundamental.hpp>

namespace hc::meta {
  template <typename T>
  concept is_void = __is_void(T);

  template <typename T>
  concept not_void = !__is_void(T);

  template <typename T, typename U>
  concept is_same = __is_same(T, U) && __is_same(U, T);

  template <typename T>
  concept is_integral = __is_integral(T);

  template <typename T>
  concept is_signed = is_integral<T> && __is_signed(T);

  template <typename T>
  concept is_unsigned = is_integral<T> && __is_unsigned(T);

  template <typename T>
  concept is_float = __is_floating_point(T);

  template <typename T>
  concept is_array = __is_array(T);

  template <typename T>
  concept is_enum = __is_enum(T);

  template <typename T>
  concept is_union = __is_union(T);

  template <typename T>
  concept is_struct = __is_class(T);

  // Qualifiers

  template <typename T>
  concept is_ref = __is_reference(T);

  template <typename T>
  concept not_ref = !__is_reference(T);

  template <typename T>
  concept is_ptr = __is_pointer(T);

  template <typename T>
  concept not_ptr = !__is_pointer(T);
} // namespace hc::meta
