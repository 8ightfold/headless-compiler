//===- Common/Array.hpp ---------------------------------------------===//
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

namespace hc::common {
  template <typename T, usize N>
  struct Array {
    using Type = T;
    static constexpr usize __size = N;
  public:
    static constexpr usize Size() 
    { return __size; }

    constexpr T& operator[](usize I) 
    { return this->__data[I]; }

    constexpr const T& operator[](usize I) const 
    { return this->__data[I]; }

  public:
    T __data[N] { };
  };

  template <typename T>
  struct Array<T, 0> {
    using Type = T;
    static constexpr usize __size = 0;
  public:
    static constexpr usize Size() {
      return __size;
    }
  };

  template <typename T, typename...TT>
  Array(T&&, TT&&...) -> Array<__decay(T), sizeof...(TT) + 1>;

  Array() -> Array<void, 0>;
} // namespace hc::common
